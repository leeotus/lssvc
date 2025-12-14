#include "utils/lssvc_config.h"
#include <iostream>

using namespace std;
using namespace lssvc::utils;

int main(int argc, char **argv) {
  // use absolute path
  bool ret = g_config_mgr->loadConfig("./config.json");
  if (!ret) {
    cout << "failed to load config file.\r\n";
  }
  LogInfoPtr log_info = g_config_mgr->getLogInfo();
  std::cout << "log level:" << log_info->level << " path:" << log_info->path
            << " name:" << log_info->name << std::endl;
  return 0;
}
