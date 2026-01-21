#include "live/gop_mgr.h"
#include "live/base/live_logger.h"
#include <fstream>

using namespace lssvc::mmedia;
using namespace lssvc::live;

void GopMgr::addFrame(const mmedia::PacketPtr &pkt) {
  lastest_timestamp_ = pkt->getTimestamp();
  if(pkt->isKeyFrame()) {
    gops_.emplace_back(pkt->getIndex(), pkt->getTimestamp());
    max_gop_length_ = std::max(max_gop_length_, gop_length_);
    total_gop_length_ += gop_length_;
    gop_numbers_ += 1;
    gop_length_ = 0;
  }
  gop_length_ += 1;
}

int32_t GopMgr::getMaxGopLength() const { return max_gop_length_; }

size_t GopMgr::getGopsSize() const { return gops_.size(); }

int GopMgr::getGopByLatency(int content_latency, int &latency) const {
  int got = -1;
  latency = 0;
  auto it = gops_.rbegin();
  for (; it != gops_.rend(); ++it) {
    int item_latency = lastest_timestamp_ - it->timestamp;
    if(item_latency < content_latency) {
      got = it->index;
      latency = got;
    } else {
      break;
    }
  }
  return got;
}

void GopMgr::clearExpiredGop(int min_index) {
  if(gops_.empty()) {
    return;
  }
  for (auto it = gops_.begin(); it != gops_.end();) {
    if(it->index <= min_index) {
      it = gops_.erase(it);
    } else {
      it++;
    }
  }
}

void GopMgr::printAllGops() {
  std::stringstream ss;

  ss << "all gops:";

  for (auto it = gops_.begin(); it != gops_.end(); ++it) {
    ss << "[" << it->index << " " << it->timestamp << "]";
  }
  LIVE_TRACE << ss.str() << "\r\n";
}

int64_t GopMgr::getLatestTimestamp() { return lastest_timestamp_; }
