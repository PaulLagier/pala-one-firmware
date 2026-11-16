#pragma once
#include <stdint.h>
#include <stddef.h>
#include <cstring>

namespace Companion {
constexpr size_t PacketBytes = 512;
constexpr size_t QueueCapacity = 8;
constexpr uint16_t NoOwner = 0xffff;
struct Session {
  uint16_t owner = NoOwner;
  uint32_t generation = 0, token = 0;
  bool connected() const { return owner != NoOwner && token != 0; }
};
inline bool sameSession(const Session &a, const Session &b) {
  return a.connected() && b.connected() && a.owner == b.owner && a.generation == b.generation &&
         a.token == b.token;
}
struct Packet {
  uint8_t bytes[PacketBytes]{};
  uint16_t length = 0;
  bool data = false;
  Session session;
};
struct Work {
  Packet packet;
  Session session;
  bool reset = false, overflow = false, hasPacket = false;
};

// All methods run under the transport's lock. This class does no allocation,
// filesystem/NVS work or SDK calls and can be tested independently of the radio.
class Queue {
public:
  bool connect(uint16_t owner, uint32_t token) {
    if (session_.connected() || owner == NoOwner || !token)
      return false;
    session_.owner = owner;
    session_.token = token;
    ++session_.generation;
    clear();
    reset_ = true;
    return true;
  }
  bool disconnect(uint16_t owner) {
    if (!session_.connected() || session_.owner != owner)
      return false;
    end();
    return true;
  }
  void end() {
    session_.owner = NoOwner;
    session_.token = 0;
    ++session_.generation;
    clear();
    reset_ = true;
  }
  Session session() const { return session_; }
  void discardPending() { head_ = count_ = 0; }
  bool push(const Session &expected, bool secure, const uint8_t *bytes, size_t length, bool data) {
    if (!secure || !sameSession(expected, session_) || !bytes || !length || length > PacketBytes)
      return false;
    if (overflow_)
      return false;
    if (count_ == QueueCapacity) {
      // Fail the whole pending window rather than silently dropping a chunk.
      head_ = count_ = 0;
      overflow_ = reset_ = true;
      return false;
    }
    auto &packet = packets_[(head_ + count_) % QueueCapacity];
    std::memcpy(packet.bytes, bytes, length);
    packet.length = uint16_t(length);
    packet.data = data;
    packet.session = session_;
    ++count_;
    return true;
  }
  Work take() {
    Work work;
    work.session = session_;
    work.reset = reset_;
    work.overflow = overflow_;
    reset_ = overflow_ = false;
    if (count_ && session_.connected()) {
      work.packet = packets_[head_];
      head_ = (head_ + 1) % QueueCapacity;
      --count_;
      work.hasPacket = true;
    }
    return work;
  }

private:
  void clear() {
    head_ = count_ = 0;
    overflow_ = false;
  }
  Session session_;
  Packet packets_[QueueCapacity];
  size_t head_ = 0, count_ = 0;
  bool reset_ = false, overflow_ = false;
};
} // namespace Companion
