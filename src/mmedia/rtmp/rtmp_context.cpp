#include "mmedia/rtmp/rtmp_context.h"
#include "mmedia/base/bytes_reader.h"
#include "mmedia/base/mmedia_logger.h"
#include "mmedia/rtmp/rtmp_handler.h"
#include "mmedia/rtmp/rtmp_handshake.h"

using namespace lssvc::mmedia;
using namespace lssvc::network;

RtmpContext::RtmpContext(const network::TcpConnectionPtr &conn,
                         RtmpHandler *handler, bool client)
    : handshake_(conn, client), connection_(conn), rtmp_handler_(handler) {}

int32_t RtmpContext::parse(LSSMsgBuffer &buf) {
  int ret = 0;
  if (state_ == kRtmpHandShake) {
    ret = handshake_.handShake(buf);
    if (ret == 0) {
      state_ = kRtmpMessage;
      if (buf.readableBytes() > 0) {
        parse(buf); // recursivly parse buffer
      }
    } else if (ret == -1) {
      RTMP_ERROR << "rtmp handshake error";
    } else if (ret == 2) {
      state_ = kRtmpWaitingDone;
    }
  } else if (state_ == kRtmpMessage) {
    return parseMessage(buf);
  }
  return ret;
}

void RtmpContext::onWriteComplete() {
  if(state_ == kRtmpHandShake) {
    handshake_.writeComplete();
  } else if (state_ == kRtmpWaitingDone) {
    state_ = kRtmpMessage;
  } else if (state_ == kRtmpMessage) {
    // TODO
  }
}

void RtmpContext::startHandShake() {
  handshake_.start();
}

int32_t RtmpContext::parseMessage(LSSMsgBuffer &buf) {
  uint8_t fmt = 0;
  uint32_t csid = 0;
  uint32_t parsed = 0;
  uint32_t total_bytes = buf.readableBytes();

  while(total_bytes > 1) {
    const char *pos = buf.peek();

    // parse basic header
    fmt = (*pos >> 6) & 0x03;
    csid = (*pos) & 0x3f;

    // record the bytes currently parsed
    parsed += 1;

    if(csid == 0) {
      // type 0
      if(total_bytes < 2) {
        // length of basic header is 2 bytes
        return 1;
      }
      csid = 64;
      csid += *((uint8_t *)(pos + parsed));
      parsed += 1;
    } else if(csid == 1) {
      if(total_bytes < 3) {
        // length of basic header is 3 bytes
        return 1;
      }
      csid = 64;
      csid += *((uint8_t *)(pos + parsed));
      parsed += 1;
      csid += *((uint8_t *)(pos + parsed)) * 256;
      parsed += 1;
    }

    // get the length of the remaining data
    int size = total_bytes - parsed;
    /**
     * fmt == 0, message header = 11B
     * fmt == 1, message header = 7B
     * fmt == 2, message header = 3B
     */
    if (size == 0 || (fmt == 0 && size < 11) || (fmt == 1 && size < 7) ||
        (fmt == 2 && size < 3)) {
      return 1;
    }

    uint32_t msg_len = 0, msg_sid = 0, timestamp = 0;
    uint8_t msg_type = 0;
    int32_t ts = 0;

    // get the previous header
    RtmpMsgHeaderPtr &prev = in_message_headers_[csid];
    if(!prev) {
      // the first chunk
      prev = std::make_shared<RtmpMsgHeader>();
    }
    if(fmt == kRtmpFmt0) {
      ts = BytesReader::readUint24T(pos + parsed);
      parsed += 3;
      in_deltas_[csid] = 0;
      timestamp = ts;
      msg_len = BytesReader::readUint24T(pos + parsed);
      parsed += 3;
      msg_type = BytesReader::readUint8T(pos + parsed);
      parsed += 1;
      memcpy(&msg_sid, pos + parsed, 4);
      parsed += 4;
    } else if(fmt == kRtmpFmt1) {
      ts = BytesReader::readUint24T(pos + parsed);
      parsed += 3;
      in_deltas_[csid] = ts;
      timestamp = ts + prev->timestamp;
      msg_len = BytesReader::readUint24T(pos + parsed);
      parsed += 3;
      msg_type = BytesReader::readUint8T(pos + parsed);
      parsed += 1;
      msg_sid = prev->msg_sid;
    } else if(fmt == kRtmpFmt2) {
      ts = BytesReader::readUint24T(pos + parsed);
      parsed += 3;
      in_deltas_[csid] = ts;
      timestamp = ts + prev->timestamp;
      msg_len = prev->msg_len;
      msg_type = prev->msg_type;
      msg_sid = prev->msg_sid;
    } else if(fmt == kRtmpFmt3) {
      timestamp = in_deltas_[csid] + prev->timestamp;
      msg_len = prev->msg_len;
      msg_type = prev->msg_type;
      msg_sid = prev->msg_sid;
    }

    bool ext = (ts == 0xffffff); // check whether timestamp field is over-flow
    if(fmt == kRtmpFmt3) {
      ext = in_ext_[csid];
    }
    in_ext_[csid] = ext;
    if(ext) {
      // extended timestamp field
      if(total_bytes - parsed < 4) {
        return 1;
      }
      ts = BytesReader::readUint32T(pos + parsed);
      parsed += 4;
      if(fmt != kRtmpFmt0) {
        timestamp = ts + prev->timestamp;
        in_deltas_[csid] = ts;
      }
    }

    PacketPtr &packet = in_packets_[csid];
    if(!packet) {
      packet = Packet::newPacket2(msg_len);
    }
    RtmpMsgHeaderPtr header = packet->getExt<RtmpMsgHeader>();
    if(!header) {
      header = std::make_shared<RtmpMsgHeader>();
      packet->setExt(header);
    }
    header->cs_id = csid;
    header->msg_len = msg_len;
    header->msg_sid = msg_sid;
    header->msg_type = msg_type;
    header->timestamp = timestamp;

    int bytes = std::min(packet->getSpace(), in_chunk_size_);
    if(total_bytes - parsed < bytes) {
      return 1;
    }
    const char *body = packet->data() + packet->getPacketSize();
    memcpy((void *)body, pos + parsed, bytes);
    packet->updatePacketSize(bytes);
    parsed += bytes;

    buf.retrieve(parsed);
    total_bytes -= parsed;

    prev->cs_id = csid;
    prev->msg_len = msg_len;
    prev->msg_sid = msg_sid;
    prev->msg_type = msg_type;
    prev->timestamp = timestamp;

    if(packet->getSpace() == 0) {
      // received all data
      packet->setPacketType(msg_type);
      packet->setTimestamp(timestamp);
      messageComplete(std::move(packet));
      packet.reset();
    }
  }
  return 1;
}

void RtmpContext::messageComplete(PacketPtr &&data) {
  // TODO: parse audio & video data
  RTMP_TRACE << "receive message type" << data->getPacketType() << ", length:" << data->getPacketSize();
}
