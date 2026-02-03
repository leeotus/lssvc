#ifndef __FLV_CONTEXT_H__
#define __FLV_CONTEXT_H__

#include "mmedia/base/mmedia_handler.h"
#include "mmedia/base/packet.h"
#include "network/net/lssvc_connection.h"
#include "network/net/lssvc_tcpconn.h"
#include <cstdint>
#include <cstring>
#include <list>
#include <memory>
#include <string>

namespace lssvc::mmedia {

class FlvContext {
public:
  /**
   * @brief construct a new flv context object
   * @param conn [in] peer tcp connection
   * @param handler [in] servicing upper-level flv-context server/client
   */
  FlvContext(const network::TcpConnectionPtr &conn, MMediaHandler *handler);
  ~FlvContext() = default;

  /**
   * @brief send http-flv header message
   * @param has_video [in] whether there exists video data or not
   * @param has_audio [in] whetehr there exists audio data or not
   */
  void sendFlvHttpHeader(bool has_video, bool has_audio);

  // @brief write flv header to the peer connection
  void writeFlvHeader(bool has_video, bool has_audio);

  bool buildFlvFrame(PacketPtr &pkt, uint32_t timestamp);

  // @brief send data to the peer connection
  void send();

  void writeComplete(const network::TcpConnectionPtr& conn);

  // @brief sending or not
  bool ready() const;

private:
  // @brief return the type of rtmp package
  char getRtmpPacketType(PacketPtr &pkt);

  std::list<network::BufferNodePtr> bufs_; // stores message data to be sent
  std::list<PacketPtr> out_packets_;
  network::TcpConnectionPtr connection_; // peer connection
  uint32_t previous_size_{0};            // previous tag's length
  std::string http_header_;              // http header
  char out_buffer_[512];                 // store flv tag
  char *current_{nullptr};               // for out_buffer
  bool sending_{false};
  MMediaHandler *handler_{nullptr};
};

} // namespace lssvc::mmedia

#endif
