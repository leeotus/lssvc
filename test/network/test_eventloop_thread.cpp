/**
 * @file test_eventloop_thread.cpp
 * @author leeotus (leeotus@163.com)
 * @brief test whether LSSEventLoopThread class works corrent or not
 * @version 1.0.0
 */

#include "network/net/lssvc_event.h"
#include "network/net/lssvc_eventloop.h"
#include "network/net/lssvc_eventloop_thread.h"
#include "network/net/lssvc_eventloop_threadpool.h"
#include "utils/lssvc_time.h"
#include <iostream>
#include <thread>

using namespace lssvc::utils;
using namespace lssvc::network;
using namespace std;

int main(int argc, char **argv) {
  LSSEventLoopThreadPool pool(2, 0, 2);
  pool.start(); // begin to work
  cout << "thread id:" << std::this_thread::get_id() << "\r\n"; // get the thread id
  vector<LSSEventLoop*> loops = pool.getLoops();
  for(auto &e : loops) {
    e->enqueueTask([&e]() {
      std::cout << "loop:" << e << ", thread id:" << std::this_thread::get_id() << "\r\n";
    });
  }
  return 0;
}
