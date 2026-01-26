#ifndef __GOP_MGR_H__
#define __GOP_MGR_H__

#include "mmedia/base/packet.h"
#include <cstdint>
#include <memory>
#include <vector>

namespace lssvc::live {

struct GopItemInfo {
  int32_t index;
  int64_t timestamp;
  GopItemInfo(int32_t i, int64_t t) : index(i), timestamp(t) {}
};

class GopMgr {
public:
  GopMgr() = default;
  ~GopMgr() {};

  /**
   * @brief add key frames into the gops
   * @param pkt [in] key frames (IDR)
   */
  void addFrame(const mmedia::PacketPtr &pkt);

  int32_t getMaxGopLength() const;

  // @brief get the size of stored gops
  size_t getGopsSize() const;

  /**
   * @brief
   * @param content_latency [in]
   * @param latency [out]
   * @return int
   */
  int getGopByLatency(int content_latency, int &latency) const;

  /**
   * @brief clear all expired gops of which the index is less than the input one
   * @param min_index [in] threshold index
   */
  void clearExpiredGop(int min_index);

  // @brief dump all gop information
  void printAllGops();

  // @brief return the latest timestamp
  int64_t getLatestTimestamp() const;

private:
  std::vector<GopItemInfo> gops_;
  int32_t gop_length_{0}; // length of the lastest gop
  int32_t max_gop_length_{0};
  int32_t gop_numbers_{0}; // total number of gops
  int32_t total_gop_length_{0}; // total length of the gops
  int64_t lastest_timestamp_{0};
};

}   // namespace lssvc::live

#endif
