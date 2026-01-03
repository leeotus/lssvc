#include "network/base/lssvc_netlogger.h"
#include "network/dns_service.h"

#include <iostream>
#include <vector>

using namespace std;
using namespace lssvc::network;

int main(int argc, char **argv) {
  std::vector<InetAddressPtr> addrs;
  std::string baidu = "www.baidu.com";
  g_dns_service->addHost(baidu);
  g_dns_service->start();

  std::this_thread::sleep_for(std::chrono::seconds(2));
  addrs = g_dns_service->getHostAddresses(baidu);
  for (auto &p : addrs) {
    cout << "ip:" << p->toIpWithPort() << "\r\n";
  }
  getchar();
  g_dns_service->stop();
  return 0;
}
