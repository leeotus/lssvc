#include "utils/lssvc_time.h"
#include <iostream>
#include <string>

using namespace std;
using namespace lssvc::utils;

int main(int argc, char **argv) {
  int64_t now_ms = LSSTime::nowMs();
  cout << "now_ms: " << now_ms << "\r\n";
  int64_t now = LSSTime::now();
  cout << "now: " << now << "\r\n";
  int year, month, day, hour, minute, second;
  int64_t res = LSSTime::now(year, month, day, hour, minute, second);
  cout << "res: " << res;
  cout << " year: " << year << ", month: " << month << ", day: " << day
       << ", hour: " << hour << ", minute: " << minute << ", second: " << second
       << "\r\n";
  string iso = LSSTime::getISOTime();
  cout << iso << "\r\n";
  return 0;
}
