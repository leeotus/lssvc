#include "network/base/lssvc_msgbuffer.h"
#include <assert.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/uio.h>

using namespace lssvc::network;

LSSMsgBuffer::LSSMsgBuffer(size_t len)
    : head_(kBufferOffset), cap_(len), buffer_(len + head_), tail_(head_) {}

LSSMsgBuffer::~LSSMsgBuffer() {}

const char *LSSMsgBuffer::peek() const { return begin() + head_; }

uint16_t LSSMsgBuffer::peekInt16() const {
  assert(readableBytes() >= 2);
  uint16_t rs = *(static_cast<const uint16_t *>((void *)peek()));
  return ntohs(rs);
}

uint32_t LSSMsgBuffer::peekInt32() const {
  assert(readableBytes() >= 4);
  uint32_t rl = *(static_cast<const uint32_t *>((void *)peek()));
  return ntohl(rl);
}

uint64_t LSSMsgBuffer::peekInt64() const {
  assert(readableBytes() >= 8);
  uint64_t rll = *(static_cast<const uint32_t *>((void *)peek()));
  return ntoh64(rll);
}

std::string LSSMsgBuffer::read(size_t len) {
  if (len > readableBytes()) {
    len = readableBytes();
  }
  std::string ret(peek(), len);
  return ret;
}

uint8_t LSSMsgBuffer::readInt8() {
  uint8_t ret = peekInt8();
  retrieve(1);
  return ret;
}

uint16_t LSSMsgBuffer::readInt16() {
  uint16_t ret = peekInt16();
  retrieve(2);
  return ret;
}

uint32_t LSSMsgBuffer::readInt32() {
  uint32_t ret = peekInt32();
  retrieve(4);
  return ret;
}

uint64_t LSSMsgBuffer::readInt64() {
  uint64_t ret = peekInt64();
  retrieve(8);
  return ret;
}

void LSSMsgBuffer::swap(LSSMsgBuffer &buf) noexcept {
  buffer_.swap(buf.buffer_);
  std::swap(head_, buf.head_);
  std::swap(tail_, buf.tail_);
  std::swap(cap_, buf.cap_);
}

void LSSMsgBuffer::retrieve(size_t len) {
  if (len >= readableBytes()) {
    retrieveAll();
    return;
  }
  head_ += len;
}

void LSSMsgBuffer::retrieveAll() {
  if (buffer_.size() > (cap_ * 2)) {
    buffer_.resize(cap_);
  }
  tail_ = head_ = kBufferOffset;
}

const char *LSSMsgBuffer::beginWrite() const { return begin() + tail_; }

char *LSSMsgBuffer::beginWrite() { return begin() + tail_; }

size_t LSSMsgBuffer::readableBytes() const { return tail_ - head_; }

size_t LSSMsgBuffer::writableBytes() const { return buffer_.size() - tail_; }

void LSSMsgBuffer::hasWritten(size_t len) {
  assert(len <= writableBytes());
  tail_ += len;
}

void LSSMsgBuffer::append(const LSSMsgBuffer &buf) {
  ensureWriteableBytes(buf.readableBytes());
  memcpy(&buffer_[tail_], buf.peek(), buf.readableBytes());
  tail_ += buf.readableBytes();
}

void LSSMsgBuffer::append(const char *buf, size_t len) {
  ensureWriteableBytes(len);
  memcpy(&buffer_[tail_], buf, len);
  tail_ += len;
}

void LSSMsgBuffer::appendInt16(const uint16_t s) {
  uint16_t ss = htons(s);
  append(static_cast<const char *>((void *)&ss), 2);
}
void LSSMsgBuffer::appendInt32(const uint32_t i) {
  uint32_t ii = htonl(i);
  append(static_cast<const char *>((void *)&ii), 4);
}
void LSSMsgBuffer::appendInt64(const uint64_t l) {
  uint64_t ll = hton64(l);
  append(static_cast<const char *>((void *)&ll), 8);
}

void LSSMsgBuffer::ensureWriteableBytes(size_t len) {
  if (writableBytes() >= len) {
    // have enough space for written data
    return;
  }
  if (head_ + writableBytes() >= (len + kBufferOffset)) {
    std::copy(begin() + head_, begin() + tail_, begin() + kBufferOffset);
    tail_ = kBufferOffset + (tail_ - head_);
    head_ = kBufferOffset;
    return;
  }
  // else create a new buffer
  size_t newLen;
  if ((buffer_.size() * 2) > (kBufferOffset + readableBytes() + len)) {
    newLen = buffer_.size() * 2;
  } else {
    newLen = kBufferOffset + readableBytes() + len;
  }
  LSSMsgBuffer newbuffer(newLen);
  newbuffer.append(*this);
  swap(newbuffer);
}

ssize_t LSSMsgBuffer::readFd(int fd, int *retErrno) {
  char extBuffer[8192];
  struct iovec vec[2];
  size_t writable = writableBytes();
  vec[0].iov_base = begin() + tail_;
  vec[0].iov_len = static_cast<int>(writable);
  vec[1].iov_base = extBuffer;
  vec[1].iov_len = sizeof(extBuffer);
  const int iovcnt = (writable < sizeof(extBuffer)) ? 2 : 1;
  ssize_t n = ::readv(fd, vec, iovcnt);
  if (n < 0) {
    *retErrno = errno;
  } else if (static_cast<size_t>(n) <= writable) {
    tail_ += n;
  } else {
    tail_ = buffer_.size();
    append(extBuffer, n - writable);
  }
  return n;
}
