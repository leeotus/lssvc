#ifndef __RTMP_HEADER_H__
#define __RTMP_HEADER_H__

#include <stdint.h>
#include <memory>

namespace lssvc::mmedia {

/**-------------------------------------------------------------------------------------------------------------------------
RTMP's audio and video data flow:
YUV/PCM ----> H264/AAC ----> Message ----> Chunk

RTMP chunk: Chunk Header + Chunk Data

- one possible Chunk Header format:
|<-------  Basic Header: 1Byte ------>|<---------------- Message Header: 11Bytes -------------------->|<-- may not exists-->|
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                 |                   |             |                 |             |                 |                     |
|   chunk type    |   chunk stream ID |  timestamp  | message length  | msg type id |  msg stream id  | extended timestamp  |
|                 |                   |             |                 |             |                 |                     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|<----2 bits----->|<------6 bits----->|<---3 Bytes->|<---3 Bytes----->|<--1 Byte--->|<----4 Bytes---->|<------ 4bytes ----->|

Extend Timestamp: this field depends on the timestamp or timestamp delta in the chunk message header.

the format of a chunk header depends on the chunk stream ID field.
if "csid" == 0, the length of "basic header" is 2 bytes:
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|  fmt  |       0 |   csid - 64 |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|< 2b  >|< 6bits >|<-- 1byte -->|

if "csid" == 1, the length of "basic header" is 3 bytes:
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|  fmt  |       1 |         csid - 64       |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|< 2b  >|< 6bits >|<--      2bytes       -->|

- Timestamp (3bytes): for blocks of type 0, the absolute timestamp is sent here. If the timestamp is greater than
or equal to 16777215 (2^24-1), this field MUST be 16777215, indicating that the Extended Timestamp field uses a
full 32 bits to encode the time. Otherwise, this field SHOULD represent the complete timestamp.
- Timestamp delta (3bytes): for blocks of type 1 and tyep 2, this sends the difference between the previous block
and the current block. If the delta is greater than or equal to 16777215(2^24-1), this field MUST be 16777215,
indicating that the Extended Timestamp field uses a full 32-bit timestamp delta for encoding. Otherwise, this field
SHOULD represent the actual timestamp delta.
- Message length (3bytes): for blocks of type 0 or type 1, the length of the message is sent here. Note that in
general, this is not the same as the length of the block payload. The payload length of all blocks, except the last
one, is the maximum length of the block, and the length of the last block is the remaining length of the message
(for very short messages, this may be the entire length of the message).

*-------------------------------------------------------------------------------------------------------------------------**/

enum RtmpMsgType {
  kRtmpMsgTypeChunkSize = 1,
  kRtmpMsgTypeBytesRead = 3,
  kRtmpMsgTypeUserControl,
  kRtmpMsgTypeWindowACKSize,
  kRtmpMsgTypeSetPeerBW,
  kRtmpMsgTypeAudio = 8,
  kRtmpMsgTypeVideo,
  kRtmpMsgTypeAMF3Meta = 15,
  kRtmpMsgTypeAMF3Shared,
  kRtmpMsgTypeAMF3Message,
  kRtmpMsgTypeAMFMeta,
  kRtmpMsgTypeAMFShared,
  kRtmpMsgTypeAMFMessage,
  kRtmpMsgTypeMetadata = 22,
};

// basic header - chunk type
enum RtmpFmt { kRtmpFmt0 = 0, kRtmpFmt1, kRtmpFmt2, kRtmpFmt3 };

// chunk header - chunk stream ID
enum RtmpCSID {
  kRtmpCSIDCommand = 2,
  kRtmpCSIDAMFIni = 3,
  kRtmpCSIDAudio = 4,
  kRtmpCSIDAMF = 5,
  kRtmpCSIDVideo = 6,
};

#define kRtmpMsID0 0
#define kRtmpMsID1 1

#pragma pack(push)
#pragma pack(1)
struct RtmpMsgHeader {
  uint32_t cs_id{0};     // chunk stream id
  uint32_t timestamp{0}; // timestamp (delta)
  uint32_t msg_len{0};   // message length
  uint8_t msg_type{0};   // message type id
  uint32_t msg_sid{0};   // message stream id
  RtmpMsgHeader()
      : cs_id(0), timestamp(0), msg_len(0), msg_type(0), msg_sid(0) {}
};
#pragma pack()

using RtmpMsgHeaderPtr = std::shared_ptr<RtmpMsgHeader>;

}   // namespace lssvc::mmedia

#endif
