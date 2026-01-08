#include "mmedia/base/packet.h"
#include "string.h"

using namespace lssvc::mmedia;

Packet::Packet(uint32_t size)
    : type_{kPacketTypeUnknown}, size_{0}, index_{-1}, timestamp_{0},
      capacity_(size) {}

Packet::~Packet() {}

PacketPtr Packet::newPacket(uint32_t size) {
  // @TODO use vector (placement new) instead
  auto block_size = size + sizeof(Packet);
  Packet *pkt = (Packet *)new char[block_size];
  memset((void *)pkt, 0, block_size);
  pkt->index_ = -1;
  pkt->type_ = kPacketTypeUnknown;
  pkt->capacity_ = size;
  pkt->ext_.reset();
  return PacketPtr(pkt, [](Packet *p) { delete[](char *) p; });
}

PacketPtr Packet::newPacket2(uint32_t size) {
  auto block_size = size + sizeof(Packet);

  // allocate a memory
  void *mem = ::operator new(block_size);

  // placement new
  Packet *pkt = new (mem) Packet(size);

  pkt->ext_.reset();
  return PacketPtr(pkt, [](Packet *p) {
    p->~Packet();
    ::operator delete(p);
  });
}
