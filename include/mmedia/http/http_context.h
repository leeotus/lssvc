#ifndef __HTTP_CONTEXT_H__
#define __HTTP_CONTEXT_H__

#include "http_handler.h"
#include "http_parser.h"
#include "http_request.h"
#include "mmedia/base/packet.h"
#include "network/base/lssvc_msgbuffer.h"
#include "network/net/lssvc_eventloop.h"
#include <string>

namespace lssvc::mmedia {

enum HttpContextPostState {
  kHttpContextPostInit,
  kHttpContextPostHttp,
  kHttpContextPostHttpHeader,
  kHttpContextPostHttpBody,
  kHttpContextPostHttpStreamHeader,
  kHttpContextPostHttpStreamChunk,
  kHttpContextPostChunkHeader,
  kHttpContextPostChunkLen,
  kHttpContextPostChunkBody,
  kHttpContextPostChunkEOF,
};

class HttpContext {
public:
  /**
   * @brief construct a new HttpContext object
   * @param loop [in] the eventloop thread handling this connection
   * @param conn [in] the incoming tcp connection
   * @param handler [in] interface serving the upper layer
   */
  HttpContext(network::LSSEventLoop *loop,
              const network::TcpConnectionPtr &conn, HttpHandler *handler);

  ~HttpContext() = default;

  /**
   * @brief  parse the incoming http message in the buffer
   * @param buf [in] buffer storing the http message
   */
  int32_t parse(network::LSSMsgBuffer &buf);

  // @brief send http request to peer http connection
  bool postRequest(const std::string &data);
  bool postRequest(const std::string &header, PacketPtr &pkt);
  bool postRequest(HttpRequestPtr &req);

  bool postChunkHeader(const std::string &header);
  void postChunk(PacketPtr &chunk);
  void postEofChunk();

  bool postStreamHeader(const std::string &header);
  void postStreamChunk(PacketPtr &pkt);

  // @note for state machine transition
  void writeComplete(const network::TcpConnectionPtr &conn);

private:
  network::LSSEventLoop *loop_{nullptr};
  network::TcpConnectionPtr connection_;
  HttpParser http_parser_;
  std::string header_;   // http header
  PacketPtr out_packet_; // store the main http message
  HttpContextPostState post_state_{kHttpContextPostInit};
  bool header_sent_; // to check http header has been sent or not
  HttpHandler *handler_{nullptr};
};

} // namespace lssvc::mmedia

#endif
