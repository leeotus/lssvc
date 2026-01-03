#ifndef __LSSVC_MSG_BUFFER_H__
#define __LSSVC_MSG_BUFFER_H__

#include <algorithm>
#include <assert.h>
#include <cstddef>
#include <string.h>
#include <string>
#include <vector>

namespace lssvc::network {

static constexpr size_t kBufferOffset{8};
static constexpr size_t kBufferDefaultLength{2048};
static constexpr char CRLF[]{"\r\n"};

// @brief this class represents a memory buffer used for sending
// and receiving data
class LSSMsgBuffer {
public:
  LSSMsgBuffer(size_t len = kBufferDefaultLength);
  ~LSSMsgBuffer();

  // @brief peek the beginning of the buffer
  const char *peek() const;

  // @brief get the end of the buffer where new data can be written
  const char *beginWrite() const;
  char *beginWrite();

  // @brief get a byte value from the buffer
  uint8_t peekInt8() const {
    assert(readableBytes() >= 1);
    return *(static_cast<const uint8_t *>((void *)peek()));
  }

  // @brief get a unsigned short value from the buffer
  uint16_t peekInt16() const;

  // @brief get a unsigned int value from the buffer
  uint32_t peekInt32() const;

  // @brief get a unsigned int64 value from the buffer
  uint64_t peekInt64() const;

  // @brief get and remove some bytes from the buffer
  std::string read(size_t len);

  // @brief get and remove a byte value from the buffer
  uint8_t readInt8();

  // @brief get and remove a unsigned short value from the buffer
  uint16_t readInt16();

  // @brief get and remove a unsigned int value from the buffer
  uint32_t readInt32();

  // @brief get and remove a unsigned int64 value from the buffer
  uint64_t readInt64();

  // @brief read data from a file description (usually a socket fd) and put it
  // into the buffer
  ssize_t readFd(int fd, int *retErrno);

  // @brief find the position of the buffer where the CRLF is found
  const char *findCRLF() const {
    const char *crlf = std::search(peek(), beginWrite(), CRLF, CRLF + 2);
    return crlf == beginWrite() ? nullptr : crlf;
  }

  // @brief swap the buffer with another
  void swap(LSSMsgBuffer &buf) noexcept;

  void append(const LSSMsgBuffer &buf);
  template <int N> void append(const char (&buf)[N]) {
    assert(strnlen(buf, N) == N - 1);
    append(buf, N - 1);
  }
  void append(const char *buf, size_t len);
  void append(const std::string &buf) { append(buf.c_str(), buf.length()); }

  // @brief append a byte value to the end of the buffer
  void appendInt8(const uint8_t b);

  // @brief append a unsigned short value to the end of the buffer
  void appendInt16(const uint16_t s);

  // @brief append a unsigned int value to the end of the buffer
  void appendInt32(const uint32_t i);

  // @brief append a unsigned int64 value to the end of the buffer
  void appendInt64(const uint64_t l);

  // @brief remove all data in the buffer
  void retrieveAll();

  // @brief remove some bytes in the buffer
  void retrieve(size_t len);

  // @brief return the size of the data in the buffer
  size_t readableBytes() const;

  // @brief return the size of the empty part in the buffer
  size_t writableBytes() const;

  // @brief make sure the buffer has enough spaces to write data
  void ensureWriteableBytes(size_t len);

  // @brief move the write pointer forward when the new data has been written to
  // the buffer
  void hasWritten(size_t len);

  // @brief move the write pointers backward to remove data in the buffer
  void unWrite(size_t offset) {
    assert(readableBytes() >= offset);
    tail_ -= offset;
  }

  // @brief access a byte in the buffer
  const char &operator[](size_t offset) const {
    assert(readableBytes() >= offset);
    return peek()[offset];
  }

  char &operator[](size_t offset) {
    assert(readableBytes() >= offset);
    return begin()[head_ + offset];
  }

private:
  size_t head_;
  size_t tail_;
  std::vector<char> buffer_;

  size_t cap_; // capacity

  const char *begin() const { return &buffer_[0]; }
  char *begin() { return &buffer_[0]; }
};

inline void swap(LSSMsgBuffer &one, LSSMsgBuffer &two) noexcept {
  one.swap(two);
}

inline uint64_t hton64(uint64_t n) {
  static const int one = 1;
  static const char sig = *(char *)&one;
  if (sig == 0) {
    return n;
  }
  char *ptr = reinterpret_cast<char *>(&n);
  std::reverse(ptr, ptr + sizeof(uint64_t));
  return n;
}

inline uint64_t ntoh64(uint64_t n) { return hton64(n); }

} // namespace lssvc::network

#endif
