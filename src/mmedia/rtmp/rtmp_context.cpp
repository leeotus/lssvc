#include "mmedia/rtmp/rtmp_context.h"
#include "mmedia/base/bytes_reader.h"
#include "mmedia/base/bytes_writer.h"
#include "mmedia/base/mmedia_logger.h"
#include "mmedia/rtmp/amf/amf_object.h"
#include "mmedia/rtmp/rtmp_handler.h"
#include "mmedia/rtmp/rtmp_handshake.h"

using namespace lssvc::mmedia;
using namespace lssvc::network;

RtmpContext::RtmpContext(const network::TcpConnectionPtr &conn,
                         RtmpHandler *handler, bool client)
    : handshake_(conn, client), connection_(conn), rtmp_handler_(handler) {}

int32_t RtmpContext::parse(LSSMsgBuffer &buf) {
  int32_t ret = 0;
  if (state_ == kRtmpHandShake) {
    ret = handshake_.handShake(buf);
    if (ret == 0) {
      state_ = kRtmpMessage;
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
  } else if (state_ == kRtmpMessage) {
    checkAndSend();
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

  in_bytes_ += (buf.readableBytes() - last_left_);
  sendBytesRecv();

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
  RTMP_TRACE << "receive message type" << data->getPacketType()
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
    bool use_delta = !fmt0 && !prev && timestamp >= prev->timestamp &&
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
    out_sending_packets_.emplace_back(std::move(packet));
    RtmpMsgHeaderPtr &prev = out_message_headers_[header->cs_id];
    bool use_delta = !fmt0 && !prev && timestamp >= prev->timestamp &&
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
  PacketPtr pkt = Packet::newPacket2(64);
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
  PacketPtr pkt = Packet::newPacket2(64);
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
  PacketPtr pkt = Packet::newPacket2(64);
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
  pkt->setPacketSize(5);
  RTMP_DEBUG << "send bandwidth:" << ack_size_ << " to host:" << connection_->getPeerAddr().toIpWithPort();
  pushOutQueue(std::move(pkt));
}

void RtmpContext::sendBytesRecv() {
  if(in_bytes_ >= ack_size_) {
    PacketPtr pkt = Packet::newPacket2(64);
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
  PacketPtr pkt = Packet::newPacket2(64);
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
    RTMP_ERROR << "amf decode failed. host:" << connection_->getPeerAddr().toIpWithPort();
    return;
  }
  obj.dump();
}

