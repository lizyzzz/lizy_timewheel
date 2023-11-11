#include "timewheel.h"



TimeWheel::TimeWheel(int slot, int interval_time) {
  if (slot <= 0) {
    slot = 10;
  } 
  if (interval_time <= 0) {
    interval_time = 1000;
  }
  // 初始化操作
  closed = true;
  interval = std::chrono::milliseconds(interval_time);
  slots.resize(slot, set<pTask>());
}

TimeWheel::~TimeWheel() {
  Close();
  if (back_thread.joinable()) {
    back_thread.join();
  }
}

void TimeWheel::Close() {
  closed = true;
  cond.notify_one();
}

std::pair<int, int> TimeWheel::getPosAndCycle(int delay_time) {
  std::pair<int, int> result;
  // pos
  result.first = (curSlot + delay_time/interval.count()) % slots.size();
  // cycle
  result.second = delay_time / (slots.size() * interval.count());

  return result;
}

void TimeWheel::RemoveTask(const string& key) {
  pTask task_del = std::make_shared<taskElement>();
  task_del->key = key;
  task_del->task = nullptr; // 不确定有没有这个任务, 所以需要给空
  Operation op(Operation::DEL, task_del);
  
  {
    std::lock_guard<std::mutex> lk(task_lock);
    opList.push_back(std::move(op));
  }
  cond.notify_one();
}

// 确保只在后台线程执行
void TimeWheel::add_aux(pTask eTask) {
  if (keyToTaskElement.count(eTask->key)) {
    // 已经存在的任务先删除
    pTask task_del = keyToTaskElement[eTask->key];
    slots[task_del->pos].erase(task_del);
  }
  // 插入任务
  keyToTaskElement[eTask->key] = eTask;
  slots[eTask->pos].insert(eTask);
}

// 确保只在后台线程执行
void TimeWheel::del_aux(pTask eTask) {
  if (keyToTaskElement.count(eTask->key) == 0) {
    return;
  }
  pTask task_del = keyToTaskElement[eTask->key];
  // 删除任务
  keyToTaskElement.erase(eTask->key);
  slots[task_del->pos].erase(task_del);
}

void TimeWheel::Run() {
  if (!closed) {
    return;
  }

  curSlot = 0;
  closed = false;
  // 开启后台线程
  std::thread t(&TimeWheel::backGroundFunc, this);
  back_thread = std::move(t);
  
  return;
}

void TimeWheel::backGroundFunc() {
  // 此处持有的锁会在 wait_until 处释放
  auto end = std::chrono::high_resolution_clock::now() + interval;
  while (!closed) {
    {
      std::unique_lock<std::mutex> lk(task_lock);
      cond.wait_until(lk, end);

      if (this->closed) {
        return;
      }
      vector<Operation> opVec;
      opVec.reserve(opList.size());
      while (!this->opList.empty()) {
        // 在时间轮上执行操作
        Operation op = opList.front();
        opList.pop_front();
        opVec.push_back(op);
      }
      lk.unlock();  // 释放锁(锁只保护 opList)

      // 对任务进行增加或删除
      for (auto& op : opVec) {
        if (op.type == Operation::ADD) {
          add_aux(op.eTask);
        } else if (op.type == Operation::DEL) {
          del_aux(op.eTask);
        }
      }

      auto curTime = std::chrono::high_resolution_clock::now();
      vector<pTask> taskNodes;
      if (curTime >= end) {
        // 到达指定时间点
        for (auto it = slots[curSlot].begin(); it != slots[curSlot].end(); ) {
          if ((*it)->cycle > 0) {
            (*it)->cycle--;
            ++it;
            continue;
          }

          // 添加要的执行任务
          taskNodes.push_back(*it);

          // 从任务列表删除
          keyToTaskElement.erase((*it)->key);
          it = slots[curSlot].erase(it);
        }

        // 推进时间轮
        curSlot = (curSlot + 1) % slots.size();
        // 推进等待时间
        end = curTime + interval;
      }
      

      // 无锁情况下执行任务
      for (auto& taskNode : taskNodes) {
        taskNode->task();
      }
    }
    
  }
}
