#ifndef __PACKET_H__
#define __PACKET_H__

#include <memory>
#include <stdint.h>
#include <string.h>

namespace lssvc::mmedia {

// @brief different packet & frame types
enum {
  kPacketTypeVideo = 1,
  kPacketTypeAudio = 2,
  kPacketTypeMeta = 4,
  kPacketTypeMeta3 = 8,
  kFrameTypeKeyFrame = 16,
  kFrameTypeIDR = 32,
  kPacketTypeUnknown = 255,
};

class Packet;
using PacketPtr = std::shared_ptr<Packet>;

#pragma pack(push)
#pragma pack(1)
// @brief this Packet class is used for storing the media data
class Packet {
public:
  /**
   * @brief construct a new packet object
   * @param size [in] the inner size of this packet
   */
  Packet(uint32_t size);

  ~Packet();

  /**
   * @brief create a new packet
   * @param size [in] the size of the packet
   * @return PacketPtr pointer to the created packet
   */
  static PacketPtr newPacket(uint32_t size);

  // @brief use placement new instead
  static PacketPtr newPacket2(uint32_t size);

  // @brief check whether the packet contains video data
  bool isVideo() const {
    return (type_ & kPacketTypeVideo) == kPacketTypeVideo;
  }

  // @brief check whether the packet contains audio data
  bool isAudio() const { return type_ == kPacketTypeAudio; }

  bool isKeyFrame() const {
    return ((type_ & kFrameTypeKeyFrame) == kFrameTypeKeyFrame) &&
           ((type_ & kPacketTypeVideo) == kPacketTypeVideo);
  }

  bool isMeta() const { return type_ == kPacketTypeMeta; }

  bool isMeta3() const { return type_ == kPacketTypeMeta3; }

  // @brief get the size of the packet
  inline uint32_t getPacketSize() const { return size_; }

  // @brief get the current space
  inline int getSpace() const { return capacity_ - size_; }

  inline void setPacketSize(uint32_t size) { size_ = size; }

  inline void updatePacketSize(uint32_t size) { size_ += size; }

  /**
   * @brief get the extra-data and convert it into shared_ptr<T>
   * @tparam T the output type of extra-data
   */
  template <typename T> inline std::shared_ptr<T> getExt() const {
    return std::static_pointer_cast<T>(ext_);
  }

  // @brief set the extra-data
  inline void setExt(const std::shared_ptr<void> &ext) { ext_ = ext; }

  // @brief set the index of the packet
  void setIndex(int32_t index) { index_ = index; }

  // @brief get the index of the packet
  int32_t getIndex() const { return index_; }

  // @brief set the type of the packet
  void setPacketType(int32_t type) { type_ = type; }

  // @brief get the type of the packet
  int32_t getPacketType() const { return type_; }

  /**
   * @brief set the timestamp of the packet
   * @param timestamp [in] the current timestamp
   */
  void setTimestamp(uint64_t timestamp) { timestamp_ = timestamp; }

  // @brief get the timestamp of this packet
  int64_t getTimestamp() const { return timestamp_; }

  // @brief get the data of the current packet
  inline char *data() { return (char *)this + sizeof(Packet); }

private:
  int32_t type_;       // indicate the type of this packet
  uint32_t size_;      // the current size of this packet
  int32_t index_;      // index of the current packet
  uint64_t timestamp_; // the timestamp of the current packet
  int32_t capacity_;
  std::shared_ptr<void> ext_; // pointer to the extra-data
};
#pragma pack()

} // namespace lssvc::mmedia

#endif
