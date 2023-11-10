#ifndef TIMEWHEEL_H_
#define TIMEWHEEL_H_

#include <functional>
#include <string>
#include <vector>
#include <set>
#include <chrono>
#include <future>
#include <memory>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <utility>
#include <stdexcept>

using std::string;
using std::vector;
using std::set;
using std::unordered_map;
typedef std::function<void()> Task;

// 定时任务类
struct taskElement {
  Task task;  // 定时任务的函数
  int pos;    // 定时任务挂载在环状数组中的索引位置
  int cycle;  // 定时任务的延迟轮次
  string key; // 定时任务的唯一标识 

  bool operator==(const taskElement& etask) {
    return key == etask.key;
  }
  bool operator!=(const taskElement& etask) {
    return key != etask.key;
  }
  bool operator<(const taskElement& etask) {
    return key < etask.key;
  }
  bool operator>(const taskElement& etask) {
    return key > etask.key;
  }
};

// 定时器类
class TimeWheel {  
 public:
  /// @brief 时间轮构造函数
  /// @param slot 时间轮槽的数量 默认 slot 的数量是 10
  /// @param interval_time 时间轮运转的间隔(单位是 ms), 时间轮扫描间隔是 1s
  TimeWheel(int slot = 10, int interval_time = 1000);

  ~TimeWheel();


  /// @brief 添加定时任务
  /// @tparam _Callable 任务(可调用对象)模板类型
  /// @tparam ...Args 任务(可调用对象)参数模板类型
  /// @param key 定时任务唯一的 key
  /// @param delayTime 延迟时间(单位是 ms)
  /// @param _f 任务(可调用对象)
  /// @param ...args 任务(可调用对象)可变参数
  /// @return 任务(可调用对象)返回值(以future<>的形式)
  template<class _Callable, class... Args>
  auto Addtask(const string& key, int delayTime, _Callable&& _f, Args&&... args) 
    -> std::future< typename std::result_of<_Callable(Args...)>::type >
  {
    if (key.empty() || delayTime <= 0) {
      throw std::runtime_error("key is empty or delayTime is less than 0");
    }
    // 获取返回值
    using return_type = typename std::result_of<_Callable(Args...)>::type;
    // 打包任务
    std::shared_ptr< std::packaged_task<return_type()> >
    task = std::make_shared(std::bind(std::forward<_Callable>(_f), std::forward<Args>(args)...));

    std::future<return_type> res = task->get_future();

    auto func = [task]() {
      (*task)();
    };

    taskElement taskEle;
    taskEle.task = func;
    taskEle.key = key;
    // 根据时间确定该任务的位置
    std::pair<int, int> posAndCycle = getPosAndCycle(delayTime);
    taskEle.pos = posAndCycle.first;
    taskEle.cycle = posAndCycle.second;

    // 添加到任务中
    {
      std::lock_guard<std::mutex> lk(task_lock);
      // 如果任务存在,先删除
      if (keyToTaskElement.count(key)) {
        taskElement task_del = keyToTaskElement[key];
        slots[task_del.pos].erase(task_del); 
      }
      slots[taskEle.pos].insert(std::move(taskEle));
    }

    return res;
  }

  // 删除定时任务
  void RemoveTask(const string& key);

  // 以后台线程的方式启动定时器执行函数
  void Run();

 private:
  /// @brief 根据延迟时间获取位置和循环次数
  /// @param delay_time 延迟时间(ms)
  /// @return (pos, cycle)
  std::pair<int, int> getPosAndCycle(int delay_time);

  std::thread back_thread; // 后台线程
  std::mutex task_lock;    // 互斥量

  std::chrono::duration<int, std::milli> interval; // 时间轮运行时间间隔(单位时毫秒)
  int curSlot;  // 当前遍历到的环状数组的索引

  // 通过vector组成环状数组, 通过遍历数组的方式实现时间轮
  // 当定时任务较多时, 每个 slot 内可能有多个定时任务, 通过 set 组装
  vector<set<taskElement>> slots; 

  // 定时任务 key 到任务节点的映射, 便于在 list 中删除节点
  unordered_map<string, taskElement> keyToTaskElement;

  bool closed;  // 停止的标记

};





#endif