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
  pTask task_del = keyToTaskElement[key];
  Operation op(Operation::SUB, task_del);
  
  {
    std::lock_guard<std::mutex> lk(task_lock);
    opList.push_back(std::move(op));
  }
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
  std::unique_lock<std::mutex> lk(task_lock);
  while (!closed) {
    auto end = std::chrono::high_resolution_clock::now() + interval;
    for (auto it = slots[curSlot].begin(); it != slots[curSlot].end(); ) {
      if ((*it)->cycle > 0) {
        (*it)->cycle--;
        ++it;
        continue;
      }

      // 执行任务
      pTask taskNode = *it;
      it = slots[curSlot].erase(it);
      // 不要给耗时过长的定时任务, 否则会导致线程阻塞
      taskNode->task();
    }

    // 推进时间轮
    curSlot = (curSlot + 1) % slots.size();

    // 等待并释放锁
    cond.wait_until(lk, end, [this] {
      return this->closed;
    });
  }
}
