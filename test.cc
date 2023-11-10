#include "timewheel.h"
#include <iostream>
#include <string>
using std::cout;
using std::endl;

int func(const string& str, int i) {
  cout << "str: " << str << " i = " << i << endl;
  return i;  
}

int main(int argc, char const *argv[])
{
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


  std::this_thread::sleep_for(std::chrono::minutes(1));

  return 0;

}
