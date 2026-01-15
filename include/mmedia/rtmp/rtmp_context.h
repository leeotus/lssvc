#ifndef __RTMP_CONTEXT_H__
#define __RTMP_CONTEXT_H__

#include "amf/amf_object.h"
#include "mmedia/base/packet.h"
#include "network/base/lssvc_msgbuffer.h"
#include "network/net/lssvc_connection.h"
#include "network/net/lssvc_tcpconn.h"
#include "rtmp_handler.h"
#include "rtmp_handshake.h"
#include "rtmp_header.h"

#include <unordered_map>
#include <memory>
#include <list>

#define RTMP_BUFFER_SIZE 4096
#define RTMP_SINGLE_MAX 10
#define RTMP_APP_INDEX 2

namespace lssvc::mmedia {

enum RtmpContextState {
  kRtmpHandShake = 0,
  kRtmpWaitingDone,  // wait util handshake done
  kRtmpMessage,
};

// user-control message
enum RtmpEventType {
  kRtmpEventTypeStreamBegin = 0,
  kRtmpEventTypeStreamEOF,
  kRtmpEventTypeStreamDry,
  kRtmpEventTypeSetBufferLength,
  kRtmpEventTypeStreamsRecorded,
  kRtmpEventTypePingRequest,
  kRtmpEventTypePingResponse,
};

using CommandFunc = std::function<void(AMFObject &)>;

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

  /**
   * @brief after receiving the whole message from the connection, execute this
   * function to handle message
   * @param data [in] the whole RTMP message
   */
  void messageComplete(PacketPtr &&data);

  /**
   * @brief split the data packet into chunks
   * @param packet [in] the packet to be sent
   * @param timestamp [in] timestamp of this packet
   * @param fmt0 [in] whether current chunk's fmt (in basic header) is fmt0 or not
   */
  bool buildChunk(const PacketPtr &packet, uint32_t timestamp = 0, bool fmt0 = false);

  /**
   * @brief split the data packet (@see "buildChunk") and send chunks
   * through TCP connection
   */
  void send();

  bool ready() const;

  // @brief for rtmp client, send "play"
  void play(const std::string &url);

  // @brief for rtmp client, send "publish"
  void publish(const std::string &url);

private:
  // @see "buildChunk(public)"
  bool buildChunk(PacketPtr &&packet, uint32_t timestamp = 0, bool fmt0 = false);

  void checkAndSend();

  /**
   * @brief push the packet to be sent into the waiting queue
   * @param packet [in] the packet to be sent
   */
  void pushOutQueue(PacketPtr &&packet);

  // @brief set chunk size
  void sendSetChunkSize();

  // @brief set ack window size
  void sendAckWindowSize();

  // @brief set peer bandwidth
  void sendSetPeerBandwidth();

  // @brief send the total bytes that have received
  void sendBytesRecv();

  // @brief
  /**
   * @brief send user-control message
   * @param nType [in] see "kRtmpEventType"
   * @param value1 [in] the first value
   * @param value2 [in] the second value
   * @note the number of input value depends on the type of the message
   */
  void sendUserCtrlMessage(short nType, uint32_t value1, uint32_t value2);

  // @brief send "connect" command
  void sendConnect();

  // @brief send "createStream" command
  void sendCreateStream();

  // @brief send "onStatus" command
  void sendStatus(const std::string &level, const std::string &code,
                  const std::string &desc);

  // @brief send "play" command
  void sendPlay();

  // @brief send "publish" command
  void sendPublish();

  /**
   * @brief handle incoming message (set chunk size)
   * @param pkt [in] the incoming packet containing message
   */
  void handleChunkSize(PacketPtr &pkt);

  /**
   * @brief handle incoming message (set ack window size)
   * @param pkt [in] the packet containing message
   */
  void handleAckWindowSize(PacketPtr &pkt);

  /**
   * @brief handle incoming message (stores user-control message)
   * @param pkt [in] the packet containing message
   */
  void handleUserMessage(PacketPtr &pkt);

  /**
   * @brief handle AMF/AMF3 message
   * @param pkt [in] the packet containing AMF/AMF3 message
   * @param amf3 [in] whether it is AMF3 or not
   */
  void handleAmfCommand(PacketPtr &pkt, bool amf3 = false);

  /**
   * @brief handle peer connection's "connect" command
   * @param obj [in] AMF-packaged "connect" command
   */
  void handleConnect(AMFObject &obj);

  /**
   * @brief handle peer connection's "createStream" command
   * @param obj [in] AMF-packaged "createStream" command
   */
  void handleCreateStream(AMFObject &obj);

  /**
   * @brief handle peer connections' "play" command
   * @param obj [in] AMF-packaged "play" command
   */
  void handlePlay(AMFObject &obj);

  // @brief parse incoming message to get the "name" and the "tcUrl" value
  void parseNameAndTcUrl();

  /**
   * @brief handle peer connections' "publish" command
   * @param obj [in] AMF-packaged "publish" command
   */
  void handlePublish(AMFObject &obj);

  // @brief handle "_result"
  void handleResult(AMFObject &obj);

  // @brief handle "_error"
  void handleError(AMFObject &obj);

  void setPacketType(PacketPtr &pkt);

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

  char out_buffer_[RTMP_BUFFER_SIZE];
  char *out_current_{nullptr};

  std::unordered_map<uint32_t, uint32_t> out_deltas_;

  // previous messages' header, first: csid, second: pointer to the header struct
  std::unordered_map<uint32_t, RtmpMsgHeaderPtr> out_message_headers_;

  int32_t out_chunk_size_{RTMP_BUFFER_SIZE};

  // store the packets to be sent
  std::list<PacketPtr> out_waiting_queue_;

  // store the chunks to be sent
  std::list<network::BufferNodePtr> sending_buffers_;

  // store sending packets
  std::list<PacketPtr> out_sending_packets_;

  bool sending_{false};

  // ack windown size
  int32_t ack_size_{2048 * 1000};

  // currently received bytes
  int32_t in_bytes_{0};

  int32_t last_left_{0};

  std::string app_;
  std::string tc_url_;
  std::string name_;
  std::string session_name_;
  std::string param_;
  bool is_player_{false};

  std::unordered_map<std::string, CommandFunc> commands_;

  bool is_client_{false};
};

using RtmpContextPtr = std::shared_ptr<RtmpContext>;

} // namespace lssvc::mmedia

#endif
