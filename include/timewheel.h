#ifndef TIMEWHEEL_H_
#define TIMEWHEEL_H_

#include <functional>
#include <string>
#include <vector>
#include <list>
#include <unordered_map>
#include <thread>
#include <mutex>

using std::string;
using std::vector;
using std::list;
using std::unordered_map;
typedef std::function<void()> Task;

// 定时任务类
struct taskElement {
  Task task;  // 定时任务的函数
  int pos;    // 定时任务挂载在环状数组中的索引位置
  int cycle;  // 定时任务的延迟轮次
  string key; // 定时任务的唯一标识 
};

// 定时器类
class TimeWheel {
 public:
  // 默认 slot 的数量是 10, 时间轮扫描间隔是 1s
  TimeWheel(int slot = 10, int interval_time = 1);

  ~TimeWheel();

  // 添加定时任务
  void AddTask(taskElement* taskEle);
  // 删除定时任务
  void RemoveTask(string key);

  // 以后台线程的方式启动定时器执行函数
  void Run();

 private:
  std::thread back_thread; // 后台线程
  std::mutex task_lock;    // 互斥量

  int interval; // 时间轮运行时间间隔
  int curSlot;  // 当前遍历到的环状数组的索引

  // 通过vector组成环状数组, 通过遍历数组的方式实现时间轮
  // 当定时任务较多时, 每个 slot 内可能有多个定时任务, 通过 list 组装
  vector<list<taskElement*>> slots; 

  // 定时任务 key 到任务节点的映射, 便于在 list 中删除节点
  unordered_map<string, taskElement*> keyToTaskElement;

  bool closed;  // 停止的标记

};





#endif