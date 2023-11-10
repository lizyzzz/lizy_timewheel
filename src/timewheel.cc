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
  slots.resize(slot, set<taskElement>());
}

std::pair<int, int> TimeWheel::getPosAndCycle(int delay_time) {
  std::pair<int, int> result;
  // pos
  result.first = (curSlot + delay_time/interval.count()) % slots.size();
  // cycle
  result.second = delay_time / (slots.size() * interval.count());

  return result;
}