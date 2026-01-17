#include <iostream>
#include <stdio.h>
#include <memory>
#include <thread>

#include "utils/lssvc_config.h"
#include "utils/lssvc_filemgr.h"
#include "utils/lssvc_fileutils.h"
#include "utils/lssvc_logstream.h"
#include "utils/lssvc_taskmgr.h"

using namespace lssvc::utils;

int main(int argc, char **argv) {
  // g_lsslogger->setLogLevel(kTrace);
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
  local_logger = nullptr;
  local_logger = new LSSLogger(log);

  // g_lsslogger->setLogLevel(log_info->level);

  LSSTaskPtr task4 = std::make_shared<LSSTask>(
      [](const LSSTaskPtr &task) {
        g_file_mgr->update();
        task->restart();
      },
      1000);

  g_task_mgr->add(task4);

  for (;;) {
    g_task_mgr->work();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  delete local_logger;
  return 0;
}
