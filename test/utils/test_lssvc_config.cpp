#include "gtest/gtest.h"
#include "utils/lssvc_config.h"
#include "utils/lssvc_filemgr.h"
#include "utils/lssvc_fileutils.h"
#include "utils/lssvc_logstream.h"
#include "utils/lssvc_taskmgr.h"

#include <iostream>

using namespace lssvc::utils;

LogInfoPtr ip;

LogInfoPtr loadConfigFile() {
  if(!g_config_mgr->loadConfig("./config.json")) {
    return nullptr;
  }
  return g_config_mgr->getLogInfo();
}

LogLevel getLevel(LogInfoPtr &p) {
  return p->level;
}

std::string getPath(LogInfoPtr &p) {
  return p->path;
}

std::string getName(LogInfoPtr &p) {
  return p->name;
}

RotateType getRotateType(LogInfoPtr &p) {
  return p->rotate_type;
}

// tests
TEST(LSSConfigTest, GETLEVEL) {
  int l = static_cast<int>(getLevel(ip));
  int k = static_cast<int>(kDebug);
  EXPECT_EQ(l, k);
}

TEST(LSSConfigTest, GETPATH) {
  EXPECT_EQ(getPath(ip), "./log/");
}

TEST(LSSConfigTest, GETNAME) {
  EXPECT_EQ(getName(ip), "lssvc.log");
}

TEST(LSSConfigTest, GETROTATE) {
  int t = static_cast<int>(getRotateType(ip));
  int k = static_cast<int>(kRotateHour);
  EXPECT_EQ(t, k);
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  ip = loadConfigFile();
  if(ip == nullptr) {
    std::cerr << "Failed to load config file.\r\n";
    return -1;
  }
  return RUN_ALL_TESTS();
}
