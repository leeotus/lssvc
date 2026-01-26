#include "mmedia/rtmp/rtmp_handshake.h"
#include "mmedia/base/mmedia_logger.h"
#include "utils/lssvc_time.h"
#include <cstring>
#include <random>

using namespace lssvc::mmedia;
using namespace lssvc::network;

namespace lssvc::mmedia {
void calculateDigest(const uint8_t *src, int len, int gap, const uint8_t *key,
                     int keylen, uint8_t *dst) {
  uint32_t digestLen = 0;
#if OPENSSL_VERSION_NUMBER > 0x10100000L
  HMAC_CTX *ctx;
#else
  HMAC_CTX ctx;
#endif
  HMAC_setup(ctx, key, keylen);
  if (gap <= 0) {
    HMAC_crunch(ctx, src, len);
  } else {
    HMAC_crunch(ctx, src, gap);
    HMAC_crunch(ctx, src + gap + SHA256_DIGEST_LENGTH,
                len - gap - SHA256_DIGEST_LENGTH);
  }
  HMAC_finish(ctx, dst, digestLen);
}

bool verifyDigest(uint8_t *buf, int digest_pos, const uint8_t *key,
                  size_t keyLen) {
  uint8_t digest[SHA256_DIGEST_LENGTH];
  calculateDigest(buf, 1536, digest_pos, key, keyLen, digest);

  return memcmp(&buf[digest_pos], digest, SHA256_DIGEST_LENGTH) == 0;
}

int32_t getDigestOffset(const uint8_t *buf, int off, int mod_val) {
  uint32_t offset = 0;
  const uint8_t *ptr = reinterpret_cast<const uint8_t *>(buf + off);
  uint32_t res;

  offset = ptr[0] + ptr[1] + ptr[2] + ptr[3];
  res = (offset % mod_val) + (off + 4);
  return res;
}

RtmpHandShake::RtmpHandShake(const TcpConnectionPtr &conn, bool client)
    : connection_(conn), is_client_(client) {}

void RtmpHandShake::start() {
  createC1S1(); // init: create C0/S0 + C1/S1 packet
  if(is_client_) {
    // client
    state_ = kHandShakePostC0C1; // ready to send C0 and C1 packets
    sendC1S1();
  } else {
    // server
    state_ = kHandShakeWaitC0C1; // wait for C0 and C1 packets
  }
}

int32_t RtmpHandShake::handShake(LSSMsgBuffer &buf) {
  switch (state_) {
  case kHandShakeWaitC0C1: // server state 1
  {
    if (buf.readableBytes() < kRtmpHandShakePacketSize + 1) {
      return 1;
    }
    RTMP_TRACE << "host:" << connection_->getPeerAddr().toIpWithPort()
               << ", received C0C1";
    int offset = checkC1S1(buf.peek(), kRtmpHandShakePacketSize + 1);
    if (offset >= 0) {
      // success
      state_ = kHandShakePostS0S1;
      // create C2/S2 packet
      createC2S2(buf.peek() + 1, kRtmpHandShakePacketSize, offset);
      buf.retrieve(kRtmpHandShakePacketSize + 1);
      sendC1S1(); // send S1 packet
    } else {
      // failed
      return -1;
    }
    break;
  }
  case kHandShakeWaitC2: { // server state 4
    if (buf.readableBytes() < kRtmpHandShakePacketSize) {
      return 1;
    }
    RTMP_TRACE << "host" << connection_->getPeerAddr().toIpWithPort()
               << ", received C2";
    if (checkC2S2(buf.peek(), kRtmpHandShakePacketSize)) {
      buf.retrieve(kRtmpHandShakePacketSize);
      RTMP_TRACE << "host:" << connection_->getPeerAddr().toIpWithPort()
                 << ", handshake done";
      state_ = kHandShakeDone; // server state 5
      return 0;
    } else {
      RTMP_TRACE << "host:" << connection_->getPeerAddr().toIpWithPort()
                 << ", check C2 failed";
      return -1;
    }
    break;
  }
  case kHandShakeWaitS0S1: { // client state 2
    if (buf.readableBytes() < kRtmpHandShakePacketSize + 1) {
      return 1;
    }
    RTMP_TRACE << "host" << connection_->getPeerAddr().toIpWithPort()
               << ", received S0S1\r\n";
    auto offset = checkC1S1(buf.peek(), kRtmpHandShakePacketSize + 1);
    if (offset >= 0) {

      // create C2 packet
      createC2S2(buf.peek() + 1, kRtmpHandShakePacketSize, offset);

      buf.retrieve(kRtmpHandShakePacketSize + 1);

      if (buf.readableBytes() == kRtmpHandShakePacketSize) { // S2
        // S2 packet has come, no need to wait for it
        RTMP_TRACE << "host" << connection_->getPeerAddr().toIpWithPort()
                   << ", received S2\r\n";
        state_ = kHandShakeDoning;
        buf.retrieve(kRtmpHandShakePacketSize);
        sendC2S2();
        return 0;
      } else {
        state_ = kHandShakePostC2;
        sendC2S2();
      }
    } else {
      RTMP_TRACE << "host:" << connection_->getPeerAddr().toIpWithPort() << ", check S0S1 failed";
      return -1;
    }
    break;
  }
  }
  return 1;
}

void RtmpHandShake::writeComplete() {
  switch (state_) {
  case kHandShakePostS0S1: {   // server state 2
    RTMP_TRACE << "host:" << connection_->getPeerAddr().toIpWithPort()
               << ", post S0S1";
    state_ = kHandShakePostS2; // ready to send S2 packet
    sendC2S2();
    break;
  }
  case kHandShakePostS2: {     // server state 3
    state_ = kHandShakeWaitC2; // wait for C2 packet
    RTMP_TRACE << "host:" << connection_->getPeerAddr().toIpWithPort()
               << ", post S2";
    break;
  }
  case kHandShakePostC0C1: { // client state 1, already send C0,C1 packet in the "start"
    RTMP_TRACE << "host:" << connection_->getPeerAddr().toIpWithPort()
               << ", post C0C1";
    state_ = kHandShakeWaitS0S1;
    break;
  }
  case kHandShakePostC2: {
    RTMP_TRACE << "host:" << connection_->getPeerAddr().toIpWithPort()
               << ", post C2";
    state_ = kHandShakeDone;
    break;
  }
  case kHandShakeDoning: {
    RTMP_TRACE << "host:" << connection_->getPeerAddr().toIpWithPort()
               << ", post C2";
    state_ = kHandShakeDone;
    break;
  }
  }
}

uint8_t RtmpHandShake::genRandom() {
  std::mt19937 mt{std::random_device{}()};
  std::uniform_int_distribution<> rand(0, 256);
  return rand(mt) % 256;
}

void RtmpHandShake::createC1S1() {
  for (int i = 0; i < kRtmpHandShakePacketSize + 1; ++i) {
    C1S1_[i] = genRandom();
  }
  C1S1_[0] = '\x03';       // C0/S0 packet, version
  memset(C1S1_ + 1, 0, 4); // timestamp
  if (!is_complex_handshake_) {
    // simple handshake, data in the version field is zero
    memset(C1S1_ + 5, 0, 4); // version, 4bytes
  } else {
    auto offset = getDigestOffset(C1S1_ + 1, 8);
    uint8_t *data = C1S1_ + 1 + offset; // C1S1_[0] is C0 packet
    if (is_client_) {
      // complex version of rtmp handshake
      memcpy(C1S1_ + 5, rtmp_client_ver, 4);  // client version
      calculateDigest(C1S1_ + 1, kRtmpHandShakePacketSize, offset,
                      rtmp_player_key, PLAYER_KEY_OPEN_PART_LEN, data);
    } else {
      memcpy(C1S1_ + 5, rtmp_server_ver, 4); // server version
      calculateDigest(C1S1_ + 1, kRtmpHandShakePacketSize, offset,
                      rtmp_server_key, SERVER_KEY_OPEN_PART_LEN, data);
    }
    memcpy(digest_, data, SHA256_DIGEST_LENGTH);
  }
}

void RtmpHandShake::createC2S2(const char *data, int bytes, int offset) {
  for(int i = 0; i < kRtmpHandShakePacketSize; ++i) {
    C2S2_[i] = genRandom(); // init
  }
  memcpy(C2S2_, data, 8); // data[0-4]: version, data[5-8]: timestamp
  auto now = lssvc::utils::LSSTime::now();
  const char *ts = (char *)&now;

  // big end
  C2S2_[3] = ts[0];
  C2S2_[2] = ts[1];
  C2S2_[1] = ts[2];
  C2S2_[0] = ts[3];

  if (is_complex_handshake_) {
    uint8_t digest[32];
    if (is_client_) {
      calculateDigest((const uint8_t *)(data + offset), 32, 0, rtmp_player_key,
                      sizeof(rtmp_player_key), digest);
    } else {
      calculateDigest((const uint8_t *)(data + offset), 32, 0, rtmp_server_key,
                      sizeof(rtmp_server_key), digest);
    }
    calculateDigest(C2S2_, kRtmpHandShakePacketSize - 32, 0, digest, 32,
                    &C2S2_[kRtmpHandShakePacketSize - 32]);
  }
}

int32_t RtmpHandShake::checkC1S1(const char *data, int bytes) {
  if (bytes != kRtmpHandShakePacketSize + 1) {
    // length : C0/S1(1) + C1/S1(1536)
    RTMP_ERROR << "unexpected C1S1, len = " << bytes << "bytes";
    return -1;
  }
  if (data[0] != '\x03') { // C0/S0 packet's version field
    RTMP_ERROR << "C0's version error" << data[0] << "\r\n";
    return -1;
  }
  uint32_t *version = (uint32_t *)(data + 5); // C1/S1 packet's version field
  if(*version == 0) {
    // simple handshake
    is_complex_handshake_ = false;
    return 0;
  }
  int32_t offset = -1;
  if (is_complex_handshake_) {
    // complex handshake
    uint8_t *handshake = (uint8_t *)(data + 1);
    offset = getDigestOffset(handshake, 8);
    if(is_client_) {
      // verify the inner digest of C1 packet
      if (!verifyDigest(handshake, offset, rtmp_server_key,
                        SERVER_KEY_OPEN_PART_LEN)) {
        // @note digest and key field can be swapped arbitrarily
        // the length of digest and key field is 764 bytes
        // the length of time and version field is 4 bytes
        // therefore, the offset of digest is either 8 or 764+8=772
        offset = getDigestOffset(handshake, 772, 728);
        if (!verifyDigest(handshake, offset, rtmp_server_key,
                          SERVER_KEY_OPEN_PART_LEN)) {
          // failed to verify digest
          return -1;
        }
      }
    } else {
      // S1 packet, the following operations are same as C1 packet's
      if (!verifyDigest(handshake, offset, rtmp_player_key,
                        PLAYER_KEY_OPEN_PART_LEN)) {
        offset = getDigestOffset(handshake, 772, 728);
        if (!verifyDigest(handshake, offset, rtmp_player_key,
                          PLAYER_KEY_OPEN_PART_LEN)) {
          return -1;
        }
      }
    }
  }
  return offset;
}

bool RtmpHandShake::checkC2S2(const char *data, int bytes) {
  // TODO
  return true;
}

void RtmpHandShake::sendC1S1() {
  // send 1 byte of C0 packet + length of C1/S1 packet is
  // "kRtmpHandShakePacketSize"
  connection_->send((const char *)C1S1_, kRtmpHandShakePacketSize + 1);
}

void RtmpHandShake::sendC2S2() {
  connection_->send((const char *)C2S2_, kRtmpHandShakePacketSize);
}

} // namespace lssvc::mmedia
