#include "timewheel.h"
#include <iostream>
#include <string>
using std::cout;
using std::endl;

int func(const string& str, int i) {
  cout << "str: " << str << " i = " << i << endl;
  return i;  
}
// std::mutex lock;
// std::condition_variable cv;
// void test() {
//   std::unique_lock<std::mutex> lk(lock);
//   auto end = std::chrono::high_resolution_clock::now() + std::chrono::seconds(5);
//   if (cv.wait_until(lk, end) == std::cv_status::timeout) {
//     cout << "after wait_until with no mutex and timeout" << endl;
//   } else {
//     cout << "after wait_until with no mutex and no timeout" << endl;
//   }
// }


int main(int argc, char const *argv[])
{

  // std::thread t(&test);

  // std::this_thread::sleep_for(std::chrono::seconds(2));

  // std::unique_lock<std::mutex> lk(lock);
  // // 不释放

  // cout << "main thread held mutex" << endl;

  // t.join();


  TimeWheel tw;
  tw.Run();

  for (int i = 1; i <= 50; i++) {
    string key = "No:" + std::to_string(i);
    string str = key + " on func";
    tw.Addtask(key, i * 1000, func, str, i);
  }

  tw.Addtask("No:25", 10 * 1000, func, string("No.25 add twice"), 25);

  for (int i = 41; i <= 50; i++) {
    string key = "No:" + std::to_string(i);
    tw.RemoveTask(key);
  }


  std::this_thread::sleep_for(std::chrono::seconds(5));

  tw.Close();
  std::this_thread::sleep_for(std::chrono::seconds(5));

  return 0;

}
