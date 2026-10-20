/**
 * @file src/input_latency.h
 * @brief Private version-one mouse receipt acknowledgement envelope validation.
 */
#pragma once

// standard includes
#include <array>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>

namespace input_latency {
  constexpr std::uint32_t envelope_magic = 0x550000F0;  ///< Private mouse envelope type.
  constexpr std::uint16_t ack_type = 0x55F0;  ///< Private receipt ACK control message type.

  /**
   * @brief Validated mouse input and opaque timestamp returned to the client.
   */
  struct receipt_t {
    std::span<const std::uint8_t> input;  ///< Original input packet without its envelope.
    std::array<std::uint8_t, 12> ack;  ///< Client timestamp and original packet magic, little endian.
  };

  /**
   * @brief Read a protocol integer without alignment or native-endianness assumptions.
   * @param data At least four bytes of protocol data.
   * @param big Whether the integer uses big endian byte order.
   * @return Decoded unsigned integer.
   */
  inline std::uint32_t read32(const std::uint8_t *data, bool big) {
    std::uint32_t value = 0;
    for (int i = 0; i < 4; ++i) {
      value |= std::uint32_t(data[big ? 3 - i : i]) << (8 * i);
    }
    return value;
  }

  /**
   * @brief Validate an envelope containing exactly one mouse packet.
   * @param packet Complete raw envelope, including both input headers.
   * @return Original mouse input and receipt payload, or no value for malformed input.
   */
  inline std::optional<receipt_t> parse(std::span<const std::uint8_t> packet) {
    constexpr std::size_t prefix_size = 16;
    if (packet.size() < prefix_size + 8 || read32(packet.data(), true) != packet.size() - 4 || read32(packet.data() + 4, false) != envelope_magic) {
      return std::nullopt;
    }
    auto input = packet.subspan(prefix_size);
    auto magic = read32(input.data() + 4, false);
    std::size_t expected_size;
    switch (magic) {
      case 7:
        expected_size = 12;  // NV_REL_MOUSE_MOVE_PACKET
        break;
      case 5:
        expected_size = 18;  // NV_ABS_MOUSE_MOVE_PACKET
        break;
      case 8:
      case 9:
        expected_size = 9;  // NV_MOUSE_BUTTON_PACKET
        break;
      default:
        return std::nullopt;
    }
    if (input.size() != expected_size || read32(input.data(), true) != input.size() - 4) {
      return std::nullopt;
    }
    receipt_t result {input, {}};
    std::memcpy(result.ack.data(), packet.data() + 8, 8);
    std::memcpy(result.ack.data() + 8, input.data() + 4, 4);
    return result;
  }
}  // namespace input_latency
