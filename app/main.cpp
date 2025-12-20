#include <iostream>
#include <stdio.h>
#include <thread>

#include "utils/lssvc_config.h"
#include "utils/lssvc_filemgr.h"
#include "utils/lssvc_fileutils.h"
#include "utils/lssvc_logstream.h"
#include "utils/lssvc_taskmgr.h"

using namespace lssvc::utils;

int main(int argc, char **argv) {
  // @todo pass the config.json path through argv
  if (!g_config_mgr->loadConfig("./config.json")) {
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
  g_lsslogger->setLogLevel(log_info->level);

  for (;;) {
    // @todo
  }

  return 0;
}
