#include "mmedia/rtmp/rtmp_context.h"
#include "mmedia/base/bytes_reader.h"
#include "mmedia/base/bytes_writer.h"
#include "mmedia/base/mmedia_logger.h"
#include "mmedia/rtmp/amf/amf_object.h"
#include "mmedia/rtmp/rtmp_handler.h"
#include "mmedia/rtmp/rtmp_handshake.h"
#include "utils/lssvc_string.h"

#include <functional>

using namespace lssvc::mmedia;
using namespace lssvc::network;

RtmpContext::RtmpContext(const network::TcpConnectionPtr &conn,
                         RtmpHandler *handler, bool client)
    : handshake_(conn, client), connection_(conn), rtmp_handler_(handler),
      is_client_(client) {
  commands_["connect"] =
      std::bind(&RtmpContext::handleConnect, this, std::placeholders::_1);
  commands_["createStream"] =
      std::bind(&RtmpContext::handleCreateStream, this, std::placeholders::_1);
  commands_["_result"] =
      std::bind(&RtmpContext::handleResult, this, std::placeholders::_1);
  commands_["_error"] =
      std::bind(&RtmpContext::handleError, this, std::placeholders::_1);
  commands_["play"] =
      std::bind(&RtmpContext::handlePlay, this, std::placeholders::_1);
  commands_["publish"] =
      std::bind(&RtmpContext::handlePublish, this, std::placeholders::_1);
  out_current_ = out_buffer_;
}

int32_t RtmpContext::parse(LSSMsgBuffer &buf) {
  int32_t ret = 0;
  if (state_ == kRtmpHandShake) {
    ret = handshake_.handShake(buf);
    if (ret == 0) {
      state_ = kRtmpMessage;
      if(is_client_) {
        // rtmp client:
        sendConnect();
      }
      if (buf.readableBytes() > 0) {
        return parse(buf); // recursivly parse buffer
      }
    } else if (ret == -1) {
      RTMP_ERROR << "rtmp handshake error";
    } else if (ret == 2) {
      state_ = kRtmpWaitingDone;
    }
  } else if (state_ == kRtmpMessage) {
    auto r = parseMessage(buf);
    last_left_ = buf.readableBytes();
    return r;
  }
  return ret;
}

void RtmpContext::onWriteComplete() {
  if(state_ == kRtmpHandShake) {
    handshake_.writeComplete();
  } else if (state_ == kRtmpWaitingDone) {
    state_ = kRtmpMessage;
    if(is_client_) {
      // for client:
      sendConnect();
    }
  } else if (state_ == kRtmpMessage) {
    checkAndSend();
  }
}

void RtmpContext::startHandShake() {
  handshake_.start();
}

// use wireshake to see rtmp packets
// >> tcpdump -i lo 'port 1935' -w out.cap
// then open wireshark to parse out.cap file
int32_t RtmpContext::parseMessage(LSSMsgBuffer &buf) {
  uint8_t fmt;
  uint32_t csid, msg_len = 0, msg_sid = 0, timestamp = 0;
  uint8_t msg_type = 0;
  uint32_t total_bytes = buf.readableBytes();
  int32_t parsed = 0;

  in_bytes_ += (buf.readableBytes() - last_left_);
  sendBytesRecv();

  while (total_bytes > 1) {
    const char *pos = buf.peek();
    parsed = 0;
    // Basic Header
    fmt = (*pos >> 6) & 0x03;
    csid = *pos & 0x3F;
    parsed++;

    if (csid == 0) {
      if (total_bytes < 2) {
        return 1;
      }
      csid = 64;
      csid += *((uint8_t *)(pos + parsed));
      parsed++;
    } else if (csid == 1) {
      if (total_bytes < 3) {
        return 1;
      }
      csid = 64;
      csid += *((uint8_t *)(pos + parsed));
      parsed++;
      csid += *((uint8_t *)(pos + parsed)) * 256;
      parsed++;
    }

    int size = total_bytes - parsed;
    if (size == 0 || (fmt == 0 && size < 11) || (fmt == 1 && size < 7) ||
        (fmt == 2 && size < 3)) {
      return 1;
    }

    msg_len = 0;
    msg_sid = 0;
    msg_type = 0;
    timestamp = 0;
    int32_t ts = 0;

    RtmpMsgHeaderPtr &prev = in_message_headers_[csid];
    if (!prev) {
      prev = std::make_shared<RtmpMsgHeader>();
    }

    if (fmt == kRtmpFmt0) {
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
    } else if (fmt == kRtmpFmt1) {
      ts = BytesReader::readUint24T(pos + parsed);
      parsed += 3;
      in_deltas_[csid] = ts;
      timestamp = ts + prev->timestamp;
      msg_len = BytesReader::readUint24T(pos + parsed);
      parsed += 3;
      msg_type = BytesReader::readUint8T(pos + parsed);
      parsed += 1;
      msg_sid = prev->msg_sid;
    } else if (fmt == kRtmpFmt2) {
      ts = BytesReader::readUint24T(pos + parsed);
      parsed += 3;
      in_deltas_[csid] = ts;
      timestamp = ts + prev->timestamp;
      msg_len = prev->msg_len;
      msg_type = prev->msg_type;
      msg_sid = prev->msg_sid;
    } else if (fmt == kRtmpFmt3) {
      timestamp = in_deltas_[csid] + prev->timestamp;
      msg_len = prev->msg_len;
      msg_type = prev->msg_type;
      msg_sid = prev->msg_sid;
    }

    bool ext = (ts == 0xFFFFFF);
    if (fmt == kRtmpFmt3) {
      ext = in_ext_[csid];
    }
    in_ext_[csid] = ext;
    if (ext) {
      if (total_bytes - parsed < 4) {
        return 1;
      }
      ts = BytesReader::readUint32T(pos + parsed);
      parsed += 4;
      if (fmt != kRtmpFmt0) {
        timestamp = ts + prev->timestamp;
        in_deltas_[csid] = ts;
      }
    }

    PacketPtr &packet = in_packets_[csid];
    if (!packet) {
      packet = Packet::newPacket(msg_len);
    }
    RtmpMsgHeaderPtr header = packet->getExt<RtmpMsgHeader>();
    if (!header) {
      header = std::make_shared<RtmpMsgHeader>();
      packet->setExt(header);
    }

    header->cs_id = csid;
    header->msg_len = msg_len;
    header->msg_sid = msg_sid;
    header->msg_type = msg_type;
    header->timestamp = timestamp;

    int bytes = std::min(packet->getSpace(), in_chunk_size_);
    if (total_bytes - parsed < bytes) {
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

    if (packet->getSpace()== 0) {
      packet->setPacketType(msg_type);
      packet->setTimestamp(timestamp);
      messageComplete(std::move(packet));
      packet.reset();
    }
    }
    return 1;
}

void RtmpContext::setPacketType(PacketPtr &pkt) {
  if (pkt->getPacketType() == kRtmpMsgTypeAudio) {
    pkt->setPacketType(kPacketTypeAudio);
  } else if (pkt->getPacketType() == kRtmpMsgTypeVideo) {
    pkt->setPacketType(kPacketTypeVideo);
  } else if (pkt->getPacketType() == kRtmpMsgTypeMetadata) {
    pkt->setPacketType(kPacketTypeMeta);
  } else if (pkt->getPacketType() == kRtmpMsgTypeAMF3Meta) {
    pkt->setPacketType(kPacketTypeMeta3);
  }
}

void RtmpContext::messageComplete(PacketPtr &&data) {
  RTMP_TRACE << "receive message type:" << data->getPacketType()
             << ", length:" << data->getPacketSize();
  auto type = data->getPacketType();
  switch (type) {
  case kRtmpMsgTypeChunkSize: {
    handleChunkSize(data);
    break;
  }
  case kRtmpMsgTypeBytesRead: {
    // TODO
    RTMP_TRACE << "message bytes read received.";
    break;
  }
  case kRtmpMsgTypeUserControl: {
    handleUserMessage(data);
    break;
  }
  case kRtmpMsgTypeWindowACKSize: {
    handleAckWindowSize(data);
    break;
  }
  case kRtmpMsgTypeAMF3Message: {
    handleAmfCommand(data, true);
    break;
  }
  case kRtmpMsgTypeAMFMessage: {
    handleAmfCommand(data);
    break;
  }
  case kRtmpMsgTypeAMFMeta:
  case kRtmpMsgTypeAMF3Meta:
  case kRtmpMsgTypeAudio:
  case kRtmpMsgTypeVideo:
  {
    setPacketType(data);
    // TODO: parse audio & video data
    if(rtmp_handler_) {
      rtmp_handler_->onRecv(connection_, data);
    }
    break;
  }
  default:
    RTMP_ERROR << "not supported message type:" << type;
    break;
  }
}

bool RtmpContext::buildChunk(const PacketPtr &packet, uint32_t timestamp,
                             bool fmt0) {
  RtmpMsgHeaderPtr header = packet->getExt<RtmpMsgHeader>();
  if (header) {
    out_sending_packets_.emplace_back(packet);
    RtmpMsgHeaderPtr &prev = out_message_headers_[header->cs_id];
    bool use_delta = !fmt0 && prev && timestamp >= prev->timestamp &&
                     header->msg_sid == prev->msg_sid;
    if (!prev) {
      prev = std::make_shared<RtmpMsgHeader>();
    }
    int fmt = kRtmpFmt0;
    if (use_delta) {
      fmt = kRtmpFmt1;
      timestamp -= prev->timestamp;
      if (header->msg_type == prev->msg_type &&
          header->msg_len == prev->msg_len) {
        fmt = kRtmpFmt2;
        if (timestamp == out_deltas_[header->cs_id]) {
          fmt = kRtmpFmt3;
        }
      }
    }

    char *p = out_current_;
    if (header->cs_id < 64) {
      *p++ = (char)((fmt << 6) | header->cs_id);
    } else if (header->cs_id < 64 + 256) {
      *p++ = (char)((fmt << 6) | 0);
      *p++ = (char)(header->cs_id - 64);
    } else {
      *p++ = (char)((fmt << 6) | 1);
      uint16_t cs = header->cs_id - 64;
      memcpy(p, &cs, sizeof(uint16_t));
      p += sizeof(uint16_t);
    }

    auto ts = timestamp;
    if (timestamp > 0xffffff) {
      ts = 0xffffff;
    }
    if (fmt == kRtmpFmt0) {
      p += BytesWriter::writeUint24T(p, ts);
      p += BytesWriter::writeUint24T(p, header->msg_len);
      p += BytesWriter::writeUint8T(p, header->msg_type);

      memcpy(p, &header->msg_sid, 4);
      p += 4;
      out_deltas_[header->cs_id] = 0;
    } else if (fmt == kRtmpFmt1) {
      p += BytesWriter::writeUint24T(p, ts);
      p += BytesWriter::writeUint24T(p, header->msg_len);
      p += BytesWriter::writeUint8T(p, header->msg_type);
      out_deltas_[header->cs_id] = timestamp;
    } else if (fmt == kRtmpFmt2) {
      p += BytesWriter::writeUint24T(p, ts);
      out_deltas_[header->cs_id] = timestamp;
    }

    if (ts == 0xffffff) {
      // extended timestamp
      memcpy(p, &timestamp, 4);
      p += 4;
    }

    BufferNodePtr node =
        std::make_shared<BufferNode>(out_current_, p - out_current_);
    sending_buffers_.emplace_back(std::move(node));
    out_current_ = p;

    prev->cs_id = header->cs_id;
    prev->msg_len = header->msg_len;
    prev->msg_sid = header->msg_sid;
    prev->msg_type = header->msg_type;
    if (fmt == kRtmpFmt0) {
      prev->timestamp = timestamp;
    } else {
      prev->timestamp += timestamp;
    }

    const char *body = packet->data();
    int32_t bytes_parsed = 0;
    while (true) {
      const char *chunk = body + bytes_parsed;
      int32_t left = header->msg_len - bytes_parsed;
      int32_t size = std::min(left, out_chunk_size_);

      BufferNodePtr bfnode = std::make_shared<BufferNode>((void*)chunk, size);
      sending_buffers_.emplace_back(std::move(bfnode));
      bytes_parsed += size;

      if (bytes_parsed < header->msg_len) {
        if (out_current_ - out_buffer_ >= RTMP_BUFFER_SIZE) {
          RTMP_ERROR << "RTMP buffer out of range";
          break;
        }
        char *p = out_current_;

        if (header->cs_id < 64) {
          *p++ = (char)(0xC0 | header->cs_id);
        } else if (header->cs_id < (64 + 256)) {
          *p++ = (char)(0xC0 | 0);
          *p++ = (char)(header->cs_id - 64);
        } else {
          *p++ = (char)(0xC0 | 1);
          uint16_t cs = header->cs_id - 64;
          memcpy(p, &cs, sizeof(uint16_t));
          p += sizeof(uint16_t);
        }
        if (ts == 0xffffff) {
          memcpy(p, &timestamp, 4);
          p += 4;
        }

        BufferNodePtr nheader =
            std::make_shared<BufferNode>(out_current_, p - out_current_);
        sending_buffers_.emplace_back(std::move(nheader));
        out_current_ = p;
      } else {
        break;
      }
    }
    return true;
  }
  return false;
}

void RtmpContext::send() {
  if(sending_) {
    return;
  }
  sending_ = true;
  for (int i = 0; i < RTMP_SINGLE_MAX; ++i) {
    if (out_waiting_queue_.empty()) {
      break;
    }
    PacketPtr pkt = std::move(out_waiting_queue_.front());
    out_waiting_queue_.pop_front();

    // build chunks and enqueue them into sending_buffers
    buildChunk(std::move(pkt));
  }
  connection_->send(sending_buffers_);
}

bool RtmpContext::ready() const {
  return !sending_;
}

bool RtmpContext::buildChunk(PacketPtr &&packet, uint32_t timestamp,
                             bool fmt0) {
  RtmpMsgHeaderPtr header = packet->getExt<RtmpMsgHeader>();
  if(header) {
    RtmpMsgHeaderPtr &prev = out_message_headers_[header->cs_id];
    bool use_delta = !fmt0 && prev && timestamp >= prev->timestamp &&
                     header->msg_sid == prev->msg_sid;
    if(!prev) {
      prev = std::make_shared<RtmpMsgHeader>();
    }
    int fmt = kRtmpFmt0;
    if(use_delta) {
      fmt = kRtmpFmt1;
      timestamp -= prev->timestamp;
      if(header->msg_type == prev->msg_type && header->msg_len == prev->msg_len) {
        fmt = kRtmpFmt2;
        if(timestamp == out_deltas_[header->cs_id]) {
          fmt = kRtmpFmt3;
        }
      }
    }

    char *p = out_current_;
    if(header->cs_id < 64) {
      *p++ = (char)((fmt << 6) | header->cs_id);
    } else if(header->cs_id < 64 + 256) {
      *p++ = (char)((fmt << 6) | 0);
      *p++ = (char)(header->cs_id - 64);
    } else {
      *p++ = (char)((fmt << 6) | 1);
      uint16_t cs = header->cs_id - 64;
      memcpy(p, &cs, sizeof(uint16_t));
      p += sizeof(uint16_t);
    }

    auto ts = timestamp;
    if(timestamp > 0xffffff) {
      ts = 0xffffff;
    }
    if(fmt == kRtmpFmt0) {
      p += BytesWriter::writeUint24T(p, ts);
      p += BytesWriter::writeUint24T(p, header->msg_len);
      p += BytesWriter::writeUint8T(p, header->msg_type);

      memcpy(p, &header->msg_sid, 4);
      p += 4;
      out_deltas_[header->cs_id] = 0;
    } else if(fmt == kRtmpFmt1) {
      p += BytesWriter::writeUint24T(p, ts);
      p += BytesWriter::writeUint24T(p, header->msg_len);
      p += BytesWriter::writeUint8T(p, header->msg_type);
      out_deltas_[header->cs_id] = timestamp;
    } else if(fmt == kRtmpFmt2) {
      p += BytesWriter::writeUint24T(p, ts);
      out_deltas_[header->cs_id] = timestamp;
    }

    if(ts == 0xffffff) {
      // extended timestamp
      memcpy(p, &timestamp, 4);
      p += 4;
    }

    BufferNodePtr node =
        std::make_shared<BufferNode>(out_current_, p - out_current_);
    sending_buffers_.emplace_back(std::move(node));
    out_current_ = p;

    prev->cs_id = header->cs_id;
    prev->msg_len = header->msg_len;
    prev->msg_sid = header->msg_sid;
    prev->msg_type = header->msg_type;
    if(fmt == kRtmpFmt0) {
      prev->timestamp = timestamp;
    } else {
      prev->timestamp += timestamp;
    }

    const char *body = packet->data();
    int32_t bytes_parsed = 0;
    while(true) {
      const char *chunk = body + bytes_parsed;
      int32_t left = header->msg_len - bytes_parsed;
      int32_t size = std::min(left, out_chunk_size_);

      BufferNodePtr bfnode = std::make_shared<BufferNode>((void*)chunk, size);
      sending_buffers_.emplace_back(std::move(bfnode));
      bytes_parsed += size;

      if (bytes_parsed < header->msg_len) {
        if (out_current_ - out_buffer_ >= RTMP_BUFFER_SIZE) {
          RTMP_ERROR << "RTMP buffer out of range";
          break;
        }
        char *p = out_current_;

        if (header->cs_id < 64) {
          *p++ = (char)(0xC0 | header->cs_id);
        } else if (header->cs_id < (64 + 256)) {
          *p++ = (char)(0xC0 | 0);
          *p++ = (char)(header->cs_id - 64);
        } else {
          *p++ = (char)(0xC0 | 1);
          uint16_t cs = header->cs_id - 64;
          memcpy(p, &cs, sizeof(uint16_t));
          p += sizeof(uint16_t);
        }
        if (ts == 0xffffff) {
          memcpy(p, &timestamp, 4);
          p += 4;
        }

        BufferNodePtr nheader =
            std::make_shared<BufferNode>(out_current_, p - out_current_);
        sending_buffers_.emplace_back(std::move(nheader));
        out_current_ = p;
      } else {
        break;
      }
    }
    out_sending_packets_.emplace_back(std::move(packet));
    return true;
  }
  return false;
}

void RtmpContext::checkAndSend() {
  sending_ = false;
  out_current_ = out_buffer_;
  sending_buffers_.clear();
  out_sending_packets_.clear();

  if(!out_waiting_queue_.empty()) {
    send();
  } else {
    if(rtmp_handler_) {
      rtmp_handler_->onActive(connection_);
    }
  }
}

void RtmpContext::pushOutQueue(PacketPtr &&packet) {
  out_waiting_queue_.emplace_back(std::move(packet));
  send();
}

void RtmpContext::sendSetChunkSize() {
  PacketPtr pkt = Packet::newPacket(64);
  RtmpMsgHeaderPtr header = std::make_shared<RtmpMsgHeader>();
  if(header) {
    header->cs_id = kRtmpCSIDCommand;
    header->msg_len = 0;
    header->msg_type = kRtmpMsgTypeChunkSize;
    header->timestamp = 0;
    header->msg_sid = kRtmpMsID0;
    pkt->setExt(header);
  }
  char *body = pkt->data();
  header->msg_len = BytesWriter::writeUint32T(body, out_chunk_size_);
  pkt->setPacketSize(header->msg_len);
  RTMP_DEBUG << "send chunk size: " << out_chunk_size_
             << " to host:" << connection_->getPeerAddr().toIpWithPort();
  pushOutQueue(std::move(pkt));
}

void RtmpContext::sendAckWindowSize() {
  PacketPtr pkt = Packet::newPacket(64);
  RtmpMsgHeaderPtr header = std::make_shared<RtmpMsgHeader>();
  if(header) {
    header->cs_id = kRtmpCSIDCommand;
    header->msg_len = 0;
    header->msg_type = kRtmpMsgTypeWindowACKSize;
    header->timestamp = 0;
    header->msg_sid = kRtmpMsID0;
    pkt->setExt(header);
  }
  char *body = pkt->data();
  header->msg_len = BytesWriter::writeUint32T(body, ack_size_);
  pkt->setPacketSize(header->msg_len);
  RTMP_DEBUG << "send ack size" << ack_size_ << " to host:" << connection_->getPeerAddr().toIpWithPort();
  pushOutQueue(std::move(pkt));
}

void RtmpContext::sendSetPeerBandwidth() {
  PacketPtr pkt = Packet::newPacket(64);
  RtmpMsgHeaderPtr header = std::make_shared<RtmpMsgHeader>();
  if(header) {
    header->cs_id = kRtmpCSIDCommand;
    header->msg_len = 0;
    header->msg_type = kRtmpMsgTypeSetPeerBW;
    header->timestamp = 0;
    header->msg_sid = kRtmpMsID0;
    pkt->setExt(header);
  }

  char *body = pkt->data();
  body += BytesWriter::writeUint32T(body, ack_size_);
  *body++ = 0x02;
  header->msg_len = 5;
  pkt->setPacketSize(5);
  RTMP_DEBUG << "send bandwidth:" << ack_size_ << " to host:" << connection_->getPeerAddr().toIpWithPort();
  pushOutQueue(std::move(pkt));
}

void RtmpContext::sendBytesRecv() {
  if(in_bytes_ >= ack_size_) {
    PacketPtr pkt = Packet::newPacket(64);
    RtmpMsgHeaderPtr header = std::make_shared<RtmpMsgHeader>();
    if(header) {
      header->cs_id = kRtmpCSIDCommand;
      header->msg_len = 0;
      header->msg_type = kRtmpMsgTypeBytesRead;
      header->timestamp = 0;
      header->msg_sid = kRtmpMsID0;
      pkt->setExt(header);
    }
    char *body = pkt->data();
    header->msg_len = BytesWriter::writeUint32T(body, in_bytes_);
    pkt->setPacketSize(header->msg_len);
    pushOutQueue(std::move(pkt));
    in_bytes_ = 0;
  }
}

void RtmpContext::sendUserCtrlMessage(short nType, uint32_t value1, uint32_t value2) {
  PacketPtr pkt = Packet::newPacket(64);
  RtmpMsgHeaderPtr header = std::make_shared<RtmpMsgHeader>();
  if(header) {
    header->cs_id = kRtmpCSIDCommand;
    header->msg_len = 0;
    header->msg_type = kRtmpMsgTypeUserControl;
    header->timestamp = 0;
    header->msg_sid = kRtmpMsID0;
    pkt->setExt(header);
  }
  char *body = pkt->data();
  char *p = body;
  p += BytesWriter::writeUint16T(body, nType);
  p += BytesWriter::writeUint32T(body, value1);
  if(nType == kRtmpEventTypeSetBufferLength) {
    p += BytesWriter::writeUint32T(body, value2);
  }
  pkt->setPacketSize(header->msg_len);
  RTMP_DEBUG << "send user control type:" << nType << " value:" << value1
             << ", value2:" << value2
             << " to host:" << connection_->getPeerAddr().toIpWithPort();
  pushOutQueue(std::move(pkt));
}

void RtmpContext::sendConnect() {
  sendSetChunkSize();                                       // set chunk size
  PacketPtr pkt = Packet::newPacket(DEFAULT_PACKET_SIZE);  // new a packet
  RtmpMsgHeaderPtr header = std::make_shared<RtmpMsgHeader>();
  header->cs_id = kRtmpCSIDAMFIni; // 3
  header->msg_sid = 0;
  header->msg_len = 0;
  header->msg_type = kRtmpMsgTypeAMFMessage;
  pkt->setExt(header);

  char *body = pkt->data();
  char *p = body;

  p += AMFAny::encodeString(p, "connect");
  p += AMFAny::encodeNumber(p, 1.0);
  *p++ = kAMFObject;
  p += AMFAny::encodeNamedString(p, "app", app_);
  p += AMFAny::encodeNamedString(p, "tcUrl", tc_url_);
  p += AMFAny::encodeNamedBoolean(p, "fpad", false);
  p += AMFAny::encodeNamedNumber(p, "capabilities", 31.0);
  p += AMFAny::encodeNamedNumber(p, "audioCodecs", 1639.0);
  p += AMFAny::encodeNamedNumber(p, "videoCodecs", 252.0);
  p += AMFAny::encodeNamedNumber(p, "videoFunction", 1.0);
  *p++ = 0x00;
  *p++ = 0x00;
  *p++ = 0x09;

  header->msg_len = p - body;           // set message length
  pkt->setPacketSize(header->msg_len);  // update
  RTMP_TRACE << "send connect msg_len:" << header->msg_len
             << " to host:" << connection_->getPeerAddr().toIpWithPort();
  pushOutQueue(std::move(pkt));
}

void RtmpContext::sendCreateStream() {
  PacketPtr pkt = Packet::newPacket(DEFAULT_PACKET_SIZE);
  RtmpMsgHeaderPtr header = std::make_shared<RtmpMsgHeader>();
  header->cs_id = kRtmpCSIDAMFIni;
  header->msg_sid = 0;
  header->msg_len = 0;
  header->msg_type = kRtmpMsgTypeAMFMessage;
  pkt->setExt(header);

  char *body = pkt->data();
  char *p = body;

  p += AMFAny::encodeString(p, "createStream");
  p += AMFAny::encodeNumber(p, 4.0);
  *p++ = kAMFNull;

  header->msg_len = p - body;
  pkt->setPacketSize(header->msg_len);
  RTMP_TRACE << "send createStream msg_len:" << header->msg_len
             << " to host:" << connection_->getPeerAddr().toIpWithPort();

  pushOutQueue(std::move(pkt));
}

void RtmpContext::sendStatus(const std::string &level, const std::string &code,
                             const std::string &desc) {
  PacketPtr pkt = Packet::newPacket(DEFAULT_PACKET_SIZE);
  RtmpMsgHeaderPtr header = std::make_shared<RtmpMsgHeader>();
  header->cs_id = kRtmpCSIDAMFIni;
  header->msg_sid = 1;
  header->msg_len = 0;
  header->msg_type = kRtmpMsgTypeAMFMessage;
  pkt->setExt(header);

  char *body = pkt->data();
  char *p = body;

  p += AMFAny::encodeString(p, "onStatus");
  p += AMFAny::encodeNumber(p, 0);
  *p++ = kAMFNull;
  *p++ = kAMFObject;
  p += AMFAny::encodeNamedString(p, "level", level);
  p += AMFAny::encodeNamedString(p, "code", code);
  p += AMFAny::encodeNamedString(p, "description", desc);
  *p++ = 0x00;
  *p++ = 0x00;
  *p++ = 0x09;

  header->msg_len = p - body;
  pkt->setPacketSize(header->msg_len);
  RTMP_TRACE << "send onStatus level:" << level << " code:" << code
             << " desc:" << desc
             << " to host:" << connection_->getPeerAddr().toIpWithPort();
  pushOutQueue(std::move(pkt));
}

void RtmpContext::sendPlay() {
  PacketPtr pkt = Packet::newPacket(DEFAULT_PACKET_SIZE);
  RtmpMsgHeaderPtr header = std::make_shared<RtmpMsgHeader>();
  header->cs_id = kRtmpCSIDAMFIni;
  header->msg_sid = 1;
  header->msg_len = 0;
  header->msg_type = kRtmpMsgTypeAMFMessage;
  pkt->setExt(header);

  char *body = pkt->data();
  char *p = body;

  p += AMFAny::encodeString(p, "play");
  p += AMFAny::encodeNumber(p, 0);
  *p++ = kAMFNull;
  p += AMFAny::encodeString(p, name_);
  p += AMFAny::encodeNumber(p, -1000.0);

  header->msg_len = p - body;
  pkt->setPacketSize(header->msg_len);
  RTMP_TRACE << "send play name:" << name_ << " msg_len:" << header->msg_len
             << " to host:" << connection_->getPeerAddr().toIpWithPort();
  pushOutQueue(std::move(pkt));
}

void RtmpContext::sendPublish() {
  PacketPtr pkt = Packet::newPacket(DEFAULT_PACKET_SIZE);
  RtmpMsgHeaderPtr header = std::make_shared<RtmpMsgHeader>();
  header->cs_id = kRtmpCSIDAMFIni;
  header->msg_sid = 1;
  header->msg_len = 0;
  header->msg_type = kRtmpMsgTypeAMFMessage;
  pkt->setExt(header);

  char *body = pkt->data();
  char *p = body;

  p += AMFAny::encodeString(p, "publish");
  p += AMFAny::encodeNumber(p, 5);
  *p++ = kAMFNull;
  p += AMFAny::encodeString(p, name_);
  p += AMFAny::encodeString(p, "live");

  header->msg_len = p - body;
  pkt->setPacketSize(header->msg_len);
  RTMP_TRACE << "send publish name:" << name_ << " msg_len: " << header->msg_len
             << " to host:" << connection_->getPeerAddr().toIpWithPort();
  pushOutQueue(std::move(pkt));
}

void RtmpContext::handleChunkSize(PacketPtr &pkt) {
  if(pkt->getPacketSize() >= 4) {
    auto size = BytesReader::readUint32T(pkt->data());
    RTMP_DEBUG << "receive chunk size in_chunk_size:" << in_chunk_size_
               << " update to " << size;
    in_chunk_size_ = size;  // update chunk size
  } else {
    RTMP_ERROR << "invalid chunk size packet msg_len:" << pkt->getPacketSize()
               << " host:" << connection_->getPeerAddr().toIpWithPort();
  }
}

void RtmpContext::handleAckWindowSize(PacketPtr &pkt) {
  if(pkt->getPacketSize() >= 4) {
    auto size = BytesReader::readUint32T(pkt->data());
    RTMP_DEBUG << "receive ack window size ack_size_" << ack_size_ << " update to " << size;
    ack_size_ = size; // update ack window size
  } else {
    RTMP_ERROR << "invalid ack window size packet msg_len:"
               << pkt->getPacketSize()
               << " host:" << connection_->getPeerAddr().toIpWithPort();
  }
}

void RtmpContext::handleUserMessage(PacketPtr &pkt) {
  auto len = pkt->getPacketSize();
  if (len < 6) {
    // error occurs
    RTMP_ERROR << "invalid user control packet msg_len: "
               << pkt->getPacketSize();
    return;
  }

  char *body = pkt->data();                   // get the packet
  auto type = BytesReader::readUint16T(body); // read the first 2 bytes
  auto value = BytesReader::readUint32T(body + 2);

  // log
  RTMP_TRACE << "receive user control type: " << type << " value" << value
             << " host:" << connection_->getPeerAddr().toIpWithPort();

  // TODO
  switch (type) {
  case kRtmpEventTypeStreamBegin: {
    RTMP_TRACE << "recv stream begin value" << value
               << " host:" << connection_->getPeerAddr().toIpWithPort();
    break;
  }
  case kRtmpEventTypeStreamEOF: {
    RTMP_TRACE << "recv stream eof value" << value
               << " host:" << connection_->getPeerAddr().toIpWithPort();
    break;
  }
  case kRtmpEventTypeStreamDry: {
    RTMP_TRACE << "recv stream dry value" << value
               << " host:" << connection_->getPeerAddr().toIpWithPort();
    break;
  }
  case kRtmpEventTypeSetBufferLength: {
    RTMP_TRACE << "recv set buffer length value" << value
               << " host:" << connection_->getPeerAddr().toIpWithPort();
    if (len < 10) {
      RTMP_ERROR << "invalid user control packet msg_len:"
                 << pkt->getPacketSize()
                 << " host:" << connection_->getPeerAddr().toIpWithPort();
      return;
    }
    break;
  }
  case kRtmpEventTypeStreamsRecorded: {
    RTMP_TRACE << "recv stream recoded value" << value
               << " host:" << connection_->getPeerAddr().toIpWithPort();
    break;
  }
  case kRtmpEventTypePingRequest: {
    RTMP_TRACE << "recv ping request value" << value
               << " host:" << connection_->getPeerAddr().toIpWithPort();
    sendUserCtrlMessage(kRtmpEventTypePingResponse, value, 0);
    break;
  }
  case kRtmpEventTypePingResponse: {
    RTMP_TRACE << "recv ping response value" << value
               << " host:" << connection_->getPeerAddr().toIpWithPort();
    break;
  }
  default:
    break;
  }
}

void RtmpContext::handleAmfCommand(PacketPtr &pkt, bool amf3) {
  RTMP_TRACE << "amf message len:" << pkt->getPacketSize()
             << " host:" << connection_->getPeerAddr().toIpWithPort();

  const char *body = pkt->data();
  int32_t msg_len = pkt->getPacketSize();
  if(amf3) {
    body += 1;
    msg_len -= 1;
  }
  AMFObject obj;
  if(obj.decode(body, msg_len) < 0) {
    RTMP_ERROR << "amf decode failed. host:"
               << connection_->getPeerAddr().toIpWithPort();
    return;
  }
  const std::string &method = obj.property(0)->str();
  RTMP_TRACE << "amf command:" << method
             << " host:" << connection_->getPeerAddr().toIpWithPort();
  auto it = commands_.find(method);
  if(it == commands_.end()) {
    RTMP_TRACE << "not supported method:" << method
               << " host:" << connection_->getPeerAddr().toIpWithPort();
    return;
  }
  it->second(obj);
}

void RtmpContext::handleConnect(AMFObject &obj) {
  bool amf3{false};
  tc_url_ = obj.property("tcUrl")->str(); // parse obj to get the tcUrl value
  AMFObjectPtr sub_obj = obj.property(RTMP_APP_INDEX)->object();
  if (sub_obj) {
    app_ = sub_obj->property("app")->str();

    if (sub_obj->property("objectEncoding")) {
      // if has objectEncoding field:
      amf3 = sub_obj->property("objectEncoding")->number() == 3.0;
    }
  }
  RTMP_TRACE << "received tcUrl:" << tc_url_ << " ,app:" << app_
             << " ,is amf3? " << (amf3 ? "true" : "false");

  sendAckWindowSize();
  sendSetPeerBandwidth();
  sendSetChunkSize();

  // reply _result or _error
  PacketPtr pkt = Packet::newPacket(DEFAULT_PACKET_SIZE);
  RtmpMsgHeaderPtr header = std::make_shared<RtmpMsgHeader>();
  header->cs_id = kRtmpCSIDAMFIni;
  header->msg_sid = 0;
  header->msg_len = 0;
  header->msg_type = kRtmpMsgTypeAMFMessage;
  pkt->setExt(header);

  char *body = pkt->data();
  char *p = body;

  p += AMFAny::encodeString(p, "_result");
  p += AMFAny::encodeNumber(p, 1.0);
  *p++ = kAMFObject;
  p += AMFAny::encodeNamedString(p, "fmsVer", "FMS/3,0,1,123");
  p += AMFAny::encodeNamedNumber(p, "capabilities", 31);
  *p++ = 0x00;
  *p++ = 0x00;
  *p++ = 0x09;
  *p++ = kAMFObject;
  p += AMFAny::encodeNamedString(p, "level", "status");
  p += AMFAny::encodeNamedString(p, "code", "NetConnection.Connect.Success");
  p += AMFAny::encodeNamedString(p, "description", "Connection succeeded");
  p += AMFAny::encodeNamedNumber(p, "objectEncoding", amf3 ? 3.0 : 0);
  *p++ = 0x00;
  *p++ = 0x00;
  *p++ = 0x09;

  header->msg_len = p - body;   // update length of message
  pkt->setPacketSize(header->msg_len);
  RTMP_TRACE << "connect result msg_len: " << header->msg_len
             << " to host:" << connection_->getPeerAddr().toIpWithPort();

  pushOutQueue(std::move(pkt));
}

void RtmpContext::handleCreateStream(AMFObject &obj) {
  auto tran_id = obj.property(1)->number();

  PacketPtr pkt = Packet::newPacket(DEFAULT_PACKET_SIZE);
  RtmpMsgHeaderPtr header = std::make_shared<RtmpMsgHeader>();
  header->cs_id = kRtmpCSIDAMFIni;
  header->msg_sid = 0;
  header->msg_len = 0;
  header->msg_type = kRtmpMsgTypeAMFMessage;
  pkt->setExt(header);

  char *body = pkt->data();
  char *p = body;

  p += AMFAny::encodeString(p, "_result");
  p += AMFAny::encodeNumber(p, tran_id);
  *p++ = kAMFNull;

  p += AMFAny::encodeNumber(p, kRtmpMsID1);

  header->msg_len = p - body;
  pkt->setPacketSize(header->msg_len);
  RTMP_TRACE << "createStream result msg_len:" << header->msg_len
             << " to host:" << connection_->getPeerAddr().toIpWithPort();
  pushOutQueue(std::move(pkt));
}

void RtmpContext::handlePlay(AMFObject &obj) {
  auto tran_id = obj.property(1)->number();
  name_ = obj.property(3)->str();
  parseNameAndTcUrl();
  RTMP_TRACE << "received play session_name:" << session_name_
             << " param:" << param_
             << " host:" << connection_->getPeerAddr().toIpWithPort();

  is_player_ = true;
  sendUserCtrlMessage(kRtmpEventTypeStreamBegin, 1, 0);
  sendStatus("status", "NetStream.Play.Start", "Start playing");
  if(rtmp_handler_) {
    rtmp_handler_->onPlay(connection_, session_name_, param_);
  }
}

void RtmpContext::parseNameAndTcUrl() {
  auto pos = app_.find_first_of("/");
  if(pos != std::string::npos) {
    app_ = app_.substr(pos + 1);
  }
  param_.clear();
  pos = name_.find_first_of("?");
  if(pos != std::string::npos) {
    param_ = name_.substr(pos + 1);
    name_ = name_.substr(0, pos);
  }

  std::string domain;
  std::vector<std::string> list = utils::LSSString::split(tc_url_, "/");

  // TODO: parse url
  if(list.size() == 6) {
    // rtmp://ip/domain:port/app/stream
    domain = list[3];
    app_ = list[4];
    name_ = list[5];
  } else if(list.size() == 5) {
    // rtmp://domain:port/app/stream
    domain = list[2];
    app_ = list[3];
    name_ = list[4];
  }

  auto p = domain.find_first_of(":");
  if(p != std::string::npos) {
    // remove port
    domain = domain.substr(0, p);
  }

  session_name_.clear();
  session_name_ += domain;
  session_name_ += "/";
  session_name_ += app_;
  session_name_ += "/";
  session_name_ += name_;

  RTMP_TRACE << "session name: " << session_name_ << " param: " << param_
             << " host:" << connection_->getPeerAddr().toIpWithPort();
}

void RtmpContext::handlePublish(AMFObject &obj) {
  auto tran_id = obj.property(1)->number();
  name_ = obj.property(3)->str();

  RTMP_TRACE << "received publish session name:" << session_name_
             << " param: " << param_
             << " host:" << connection_->getPeerAddr().toIpWithPort();
  is_player_ = false;
  sendStatus("status", "NetStream.Publish.Start", "Start publish");
  if(rtmp_handler_) {
    rtmp_handler_->onPublish(connection_, session_name_, param_);
  }
}

void RtmpContext::handleResult(AMFObject &obj) {
  auto id = obj.property(1)->number();
  RTMP_TRACE << "received result id: " << id
             << " host:" << connection_->getPeerAddr().toIpWithPort();
  if (id == 1) {
    sendCreateStream();
  } else if (id == 4) {
    if (is_player_) {
      sendPlay();
    } else {
      sendPublish();
    }
  }
}

void RtmpContext::handleError(AMFObject &obj) {
  const std::string &description =
      obj.property(3)->object()->property("description")->str();
  RTMP_ERROR << "recv error description:" << description
             << " host:" << connection_->getPeerAddr().toIpWithPort();
  connection_->forceClose();
}

void RtmpContext::play(const std::string &url) {
  is_client_ = true;
  is_player_ = true;
  tc_url_ = url;
  parseNameAndTcUrl();
}

void RtmpContext::publish(const std::string &url) {
  is_client_ = true;
  is_player_ = false;
  tc_url_ = url;
  parseNameAndTcUrl();
}
