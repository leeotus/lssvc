#include "network/base/lssvc_inetaddress.h"
#include "gtest/gtest.h"
#include <iostream>
#include <string>
#include <vector>

using namespace lssvc::network;

/**========================================================================
 * !                              WARNING
 *  Ipv6 testcases may fail, because in the current version, 'LSSInetAddress::
 *  getIpAndPort' function (which is called in the construction of LSSInetAddress)
 *  fails to coop with ipv6 address(may surrounded with square brackets and many ':')
 *  and its port.
 *========================================================================**/

// testcases: ipv4 address with port
TEST(InetAddressTest, IPv4WithPort) {
  struct IPv4WithPortTestCase {
    std::string host;
    uint16_t expected_port;
    std::string expected_ip_prefix;
  };

  std::vector<IPv4WithPortTestCase> test_cases = {
      {"127.0.0.1:8080", 8080, "127.0.0.1"},
      {"192.168.1.100:22", 22, "192.168.1.100"},
      {"10.0.0.5:3306", 3306, "10.0.0.5"},
      {"172.16.31.200:80", 80, "172.16.31.200"},
      {"203.0.113.66:443", 443, "203.0.113.66"},
      {"127.0.0.1:0", 0, "127.0.0.1"}};

  for (const auto &tc : test_cases) {
    SCOPED_TRACE("Testing host:" + tc.host);
    LSSInetAddress addr(tc.host, false);
    EXPECT_EQ(addr.getPort(), tc.expected_port);
    EXPECT_NE(addr.getIp().find(tc.expected_ip_prefix), std::string::npos);
  }
}

// testcases: ipv6 address with port
TEST(InetAddressTest, IPv6WithPort) {
  struct IPv6WithPortTestCase {
    std::string host;
    uint16_t expected_port;
    std::string expected_ip_prefix;
  };

  std::vector<IPv6WithPortTestCase> test_cases = {
      {"[::1]:8080", 8080, "::1"},
      {"[fe80::1%lo0]:9090", 9090, "fe80::1"},
      {"[2001:db8::1]:443", 443, "2001:db8::1"},
      {"[::ffff:192.168.1.100]:80", 80, "192.168.1.100"}};

  for (const auto &tc : test_cases) {
    SCOPED_TRACE("Testing host: " + tc.host);
    LSSInetAddress addr(tc.host, true);
    EXPECT_EQ(addr.getPort(), tc.expected_port);
    EXPECT_NE(addr.getIp().find(tc.expected_ip_prefix), std::string::npos);
  }
}

TEST(InetAddressTest, PureIPv4WithoutPort) {
  // pure ipv4 address
  std::vector<std::string> ipv4_list = {
      "127.0.0.1",      // loopback address
      "192.168.0.254",  // local area network C class
      "10.255.255.255", // A class
      "172.31.255.255", // B class
      "223.5.5.5",      // public network
      "8.8.8.8",
      "127.0.0.0", // loopback address boundary
      "0.0.0.0"    // any address
  };

  for (const auto &ip : ipv4_list) {
    SCOPED_TRACE("Testing pure IPv4: " + ip);
    LSSInetAddress addr(ip);
    EXPECT_NE(addr.getIp().find(ip), std::string::npos);
    EXPECT_EQ(addr.getPort(), 0);
  }
}

TEST(InetAddressTest, PureIPv6WithoutPort) {
  // pure ipv6 address
  std::vector<std::string> ipv6_list = {"::1", "fe80::1",
                                        "2001:db8::", "::", "::ffff:8.8.8.8"};

  for (const auto &ip : ipv6_list) {
    SCOPED_TRACE("Testing pure IPv6: " + ip);
    LSSInetAddress addr(ip, true);
    EXPECT_NE(addr.getIp().find(ip.substr(0, 6)), std::string::npos);
    EXPECT_EQ(addr.getPort(), 0);
  }
}

TEST(InetAddressTest, InvalidHostFormat) {
  std::vector<std::string> invalid_host_list = {
      "192.168.1.256:80",  // ip out of range
      "192.168.1.1:65536", // port out of range
      "abc:1234",          // invalid
      "[192.168.1.1]:80",  // misuse ipv6's square brackets
      "::1:8080"           // square brackets
  };

  for (const auto &invalid_host : invalid_host_list) {
    SCOPED_TRACE("Testing invalid host: " + invalid_host);
    EXPECT_NO_THROW({
      LSSInetAddress addr(invalid_host);
      if (invalid_host.find(":65536") != std::string::npos) {
        EXPECT_NE(addr.getPort(), 65536); // 无效端口不应解析成功
      }
    });
  }
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
