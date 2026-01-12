#ifndef __RTMP_CONTEXT_H__
#define __RTMP_CONTEXT_H__

#include "mmedia/base/packet.h"
#include "network/base/lssvc_msgbuffer.h"
#include "network/net/lssvc_tcpconn.h"
#include "rtmp_handler.h"
#include "rtmp_handshake.h"
#include "rtmp_header.h"

#include <unordered_map>
#include <memory>

namespace lssvc::mmedia {

enum RtmpContextState {
  kRtmpHandShake = 0,
  kRtmpWaitingDone,  // wait util handshake done
  kRtmpMessage,
};

class RtmpContext {
public:
  RtmpContext(const network::TcpConnectionPtr &conn, RtmpHandler *handler,
              bool client = false);
  ~RtmpContext() = default;

  // @brief parse the incoming RTMP packet
  int32_t parse(network::LSSMsgBuffer &buf);

  void onWriteComplete();

  // @brief begin handshake process
  void startHandShake();

  // @brief parse the message
  int32_t parseMessage(network::LSSMsgBuffer &buf);

  // @brief execute this callback after parsing the message
  void messageComplete(PacketPtr &&data);

private:
  RtmpHandShake handshake_;
  int32_t state_{kRtmpHandShake};
  network::TcpConnectionPtr connection_;
  RtmpHandler *rtmp_handler_{nullptr};

  // store previous packets' header
  std::unordered_map<uint32_t, RtmpMsgHeaderPtr> in_message_headers_;

  std::unordered_map<uint32_t, PacketPtr> in_packets_;

  // store previous packets' timestamp delta
  std::unordered_map<uint32_t, uint32_t> in_deltas_;

  // check whether there is a extended timestamp or not
  std::unordered_map<uint32_t, bool> in_ext_;

  int32_t in_chunk_size_{128}; // chunk size, default 128B
};

using RtmpContextPtr = std::shared_ptr<RtmpContext>;

} // namespace lssvc::mmedia

#endif
