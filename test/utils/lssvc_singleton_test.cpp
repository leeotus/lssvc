#include "utils/lssvc_singleton.h"
#include <iostream>

using namespace std;
using namespace lssvc::utils;

class TestKLASS : public NonCopyable {
public:
  TestKLASS() = default;
  ~TestKLASS() = default;

  void sayHelloWorld() {
    cout << "hello world\r\n";
  }
};

#define instance LSSSingleton<TestKLASS>::getInstance()

int main(int argc, char **argv) {
  instance->sayHelloWorld();
  return 0;
}
