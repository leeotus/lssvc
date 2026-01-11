#ifndef __RTMP_HANDSHAKE_H__
#define __RTMP_HANDSHAKE_H__

#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <stdint.h>
#include <memory>

#include "network/net/lssvc_tcpconn.h"

#if OPENSSL_VERSION_NUMBER > 0x10100000L
#define HMAC_setup(ctx, key, len)                                              \
  ctx = HMAC_CTX_new();                                                        \
  HMAC_Init_ex(ctx, key, len, EVP_sha256(), 0)
#define HMAC_crunch(ctx, buf, len) HMAC_Update(ctx, buf, len)
#define HMAC_finish(ctx, dig, dlen)                                            \
  HMAC_Final(ctx, dig, &dlen);                                                 \
  HMAC_CTX_free(ctx)
#else
#define HMAC_setup(ctx, key, len)                                              \
  HMAC_CTX_Init(&ctx);                                                         \
  HMAC_Init_ex(&ctx, key, len, EVP_sha256(), 0)
#define HMAC_crunch(ctx, buf, len) HMAC_Update(&ctx, buf, len)
#define HMAC_finish(ctx, dig, dlen)                                            \
  HMAC_Final(&ctx, dig, &dlen);                                                \
  HMAC_CTX_cleanup(&ctx)
#endif

namespace lssvc {

namespace mmedia {

/**
 * @brief the complex handshake of RTMP, the following arrays are versions of
 * server's and client's RTMP packet (S1 and C1), respectively. When the version
 * field of C1/S1 is totally zero, it means the rtmp uses simple handshake
 * instead.
 */
static constexpr unsigned char rtmp_server_ver[4] = {0x0d, 0x0e, 0x0a, 0x0d};
static constexpr unsigned char rtmp_client_ver[4] = {0x0c, 0x00, 0x0d, 0x0e};

/**------------------------------------------------------------------------------
The complex handshake of RTMP -- 764 bytes digest structure
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|   offset     |    random-data     |   digest-data   |   radom-data            |
|   4 bytes    |    (offset) bytes  |     32 bytes    |   (764-4-offset) bytes  |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
use the random-data content around the 'digest-data' plus a fixed key (@see
"rtmp_play_key" for client and "rtmp_server_key" for server) to calculate the
digest via HMAC-SHA256, and the use this digest to verify with the digest-data
part
*------------------------------------------------------------------------------**/

#define PLAYER_KEY_OPEN_PART_LEN                                               \
  30 ///< length of partial key used for first client digest signing

/** Client key used for digest signing */
static const uint8_t rtmp_player_key[] = {
    'G',  'e',  'n',  'u',  'i',  'n',  'e',  ' ',  'A',  'd',  'o',
    'b',  'e',  ' ',  'F',  'l',  'a',  's',  'h',  ' ',  'P',  'l',
    'a',  'y',  'e',  'r',  ' ',  '0',  '0',  '1',

    0xF0, 0xEE, 0xC2, 0x4A, 0x80, 0x68, 0xBE, 0xE8, 0x2E, 0x00, 0xD0,
    0xD1, 0x02, 0x9E, 0x7E, 0x57, 0x6E, 0xEC, 0x5D, 0x2D, 0x29, 0x80,
    0x6F, 0xAB, 0x93, 0xB8, 0xE6, 0x36, 0xCF, 0xEB, 0x31, 0xAE};

#define SERVER_KEY_OPEN_PART_LEN                                               \
  36 ///< length of partial key used for first server digest signing

/** Key used for RTMP server digest signing */
static const uint8_t rtmp_server_key[] = {
    'G',  'e',  'n',  'u',  'i',  'n',  'e',  ' ',  'A',  'd',  'o',  'b',
    'e',  ' ',  'F',  'l',  'a',  's',  'h',  ' ',  'M',  'e',  'd',  'i',
    'a',  ' ',  'S',  'e',  'r',  'v',  'e',  'r',  ' ',  '0',  '0',  '1',

    0xF0, 0xEE, 0xC2, 0x4A, 0x80, 0x68, 0xBE, 0xE8, 0x2E, 0x00, 0xD0, 0xD1,
    0x02, 0x9E, 0x7E, 0x57, 0x6E, 0xEC, 0x5D, 0x2D, 0x29, 0x80, 0x6F, 0xAB,
    0x93, 0xB8, 0xE6, 0x36, 0xCF, 0xEB, 0x31, 0xAE};

void calculateDigest(const uint8_t *src, int len, int gap, const uint8_t *key,
                     int keylen, uint8_t *dst);

/**
 * @brief verify the digest
 * @param buf [in] stores digest
 * @param digest_pos [in] position of the digest in the buffer
 * @param key [in] fixed key
 * @param keyLen [in] length of the key
 * @return true if success
 */
bool verifyDigest(uint8_t *buf, int digest_pos, const uint8_t *key,
                  size_t keyLen);

/**
 * @brief find the digest-data within c1/s1 packet
 * @param buf [in] C1/S1 packet
 * @param off [in] offset 8 or 8+764
 * @param mod_val [in] default, 728 (764-32-4)
 * @note 764 bytes of digest, 32 bytes of digest-data, 4 bytes of offset
 */
int32_t getDigestOffset(const uint8_t *buf, int off, int mod_val = 728);

using namespace lssvc::network;

// states of during a rtmp handshake
enum RtmpHandShakeState {
  kHandShakeInit,

  // client
  kHandShakePostC0C1,
  kHandShakeWaitS0S1,
  kHandShakePostC2,
  kHandShakeWaitS2,
  kHandShakeDoning,

  // server
  kHandShakeWaitC0C1,
  kHandShakePostS0S1,
  kHandShakePostS2,
  kHandShakeWaitC2,

  kHandShakeDone
};

class RtmpHandShake;
using RtmpHandShakePtr = std::shared_ptr<RtmpHandShake>;

class RtmpHandShake {
  static constexpr int kRtmpHandShakePacketSize = 1536;
public:
  /**
   * @brief construct a new RtmpHandShake object
   * @param conn [in] tcp connection (@note rtmp uses tcp)
   * @param client [in] whether the incoming connection is a client or not
   */
  RtmpHandShake(const TcpConnectionPtr &conn, bool client=false);

  ~RtmpHandShake() = default;

  // @brief start hand shaking
  void start();

  int32_t handShake(LSSMsgBuffer &buf);

  void writeComplete();

private:
  // @brief generate a random number
  // @note use std::mt19937
  uint8_t genRandom();

  // @brief create C1/S1 packet
  void createC1S1();

  /**
   * @brief check the digest of the C1/S1
   * @param data [in] the incoming packet
   * @param bytes [in] length of the data
   * @return int32_t >0:offset of the digestm -1: error, ==0: not a complex
   * handshake
   */
  int32_t checkC1S1(const char *data, int bytes);

  // @brief send C1/S1 packet through connection
  void sendC1S1();

  /**
   * @brief create C2/S2 packet
   * @param data [in] the received C1/S1's packet
   * @param bytes [in] length of the data
   * @param offset [in] offset of digest inner the C1/S1 packet
   * @note data only contains C1/S1 packet, C0/S0 packet is not included
   */
  void createC2S2(const char *data, int bytes, int offset);

  // @brief send C2/S2 packet through connection
  void sendC2S2();

  // @brief todo verify C2/S2 packet
  bool checkC2S2(const char *data, int bytes);

  TcpConnectionPtr connection_;
  // specify the current handshake is from the client or the server
  bool is_client_{false};
  bool is_complex_handshake_{
      true}; // whether it's a complex handshake or not, if
             // it's a complex handshake then, the version
             // field (5-8 bytes) of C1 or S1 is non-empty
  // stores the digest in the C1/S1, it will be verifed in the C2/S2 packet
  uint8_t digest_[SHA256_DIGEST_LENGTH];
  uint8_t C1S1_[kRtmpHandShakePacketSize + 1]; // C0 & C1 or S0 & S1
  uint8_t C2S2_[kRtmpHandShakePacketSize];
  int32_t state_{kHandShakeInit};
};

} // namespace mmedia

} // namespace lssvc

#endif
