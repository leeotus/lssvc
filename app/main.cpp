#include <iostream>
#include <stdio.h>
#include <memory>
#include <thread>

#include "live/live_service.h"
#include "mmedia/rtmp/rtmp_handler.h"
#include "network/net/lssvc_eventloop.h"
#include "network/net/lssvc_eventloop_threadpool.h"
#include "network/tcp_server.h"
#include "utils/lssvc_config.h"
#include "utils/lssvc_filemgr.h"
#include "utils/lssvc_fileutils.h"
#include "utils/lssvc_logstream.h"
#include "utils/lssvc_taskmgr.h"

using namespace lssvc::utils;
using namespace lssvc::network;
using namespace lssvc::mmedia;
using namespace lssvc::live;

int main(int argc, char **argv) {
  local_logger = new LSSLogger();
  local_logger->setLogLevel(kTrace);

  // @todo pass the config.json path through argv
  if (!g_config_mgr->loadConfig("./config/config.json")) {
    std::cerr << "load config file failed\r\n";
    return -1;
  }
  LogInfoPtr log_info = g_config_mgr->getLogInfo();
  std::cout << "log level:" << log_info->level << " path:" << log_info->path
            << " name:" << log_info->name << " rotate:"
            << ((log_info->rotate_type == kRotateHour) ? "Hour" : "Unknow")
            << std::endl;

  // @todo let it create file automatically
  LSSFileLogPtr log = g_file_mgr->getFileLog(log_info->path + log_info->name);
  if (!log) {
    // need to check whether 'log' is nullptr (i.e., file_log_nullptr) or not.
    std::cerr << "log can't open\r\n";
    return -1;
  }
  log->setRotate(log_info->rotate_type);

  delete local_logger;
  local_logger = new LSSLogger(log);
  local_logger->setLogLevel(kWarn);

  LSSTaskPtr task4 = std::make_shared<LSSTask>(
      [](const LSSTaskPtr &task) {
        g_file_mgr->update();
        task->restart();
      },
      1000);

  g_task_mgr->add(task4);

  // live service
  gLiveService->start();

  for (;;) {
    g_task_mgr->work();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  delete local_logger;
  return 0;
}
