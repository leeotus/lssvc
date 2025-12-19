#include "network/net/lssvc_eventloop_thread.h"
#include "network/base/lssvc_netlogger.h"
#include "network/net/lssvc_event.h"
#include "network/net/lssvc_eventloop.h"
#include "network/net/lssvc_eventloop_threadpool.h"
#include "utils/lssvc_time.h"

#include <iostream>
#include <chrono>

using namespace lssvc::network;
using namespace lssvc::utils;
using namespace std;

int main(int argc, char **argv) {
  LSSEventLoopThreadPool pool(1, 0, 1);
  pool.start();
  LSSEventLoop *loop = pool.getNextLoop();
  cout << "loop: " << loop << "\r\n";
  loop->runAfter(1, [](){
    cout << "run after 1s now:" << LSSTime::now() << "\r\n";
  });
  loop->runEvery(5, [](){
    cout << "run every 5s now:" << LSSTime::getISOTime() << "\r\n";
  });
  while(true) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
  return 0;
}
