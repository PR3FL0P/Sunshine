/**
 * @file tests/unit/test_input_latency.cpp
 * @brief Regression coverage for private mouse receipt envelopes.
 */

// test includes
#include "../tests_common.h"

// standard includes
#include <vector>

// local includes
#include "src/input_latency.h"

namespace {
  /**
   * @brief Build a little-endian input envelope with a big-endian length prefix.
   * @param magic Original mouse message type.
   * @param size Original mouse packet size.
   * @return Serialized measurement envelope.
   */
  std::vector<std::uint8_t> envelope(std::uint8_t magic, std::size_t size) {
    std::vector<std::uint8_t> packet(16 + size);
    packet[3] = packet.size() - 4;
    packet[4] = 0xF0;
    packet[7] = 0x55;
    for (int i = 0; i < 8; ++i) {
      packet[8 + i] = 0x80 + i;
    }
    packet[19] = size - 4;
    packet[20] = magic;
    return packet;
  }
}  // namespace

TEST(InputLatency, AcceptsMousePacketsAndPreservesOpaqueTimestamp) {
  for (auto [magic, size] : {std::pair {7, 12}, {5, 18}, {8, 9}, {9, 9}}) {
    auto packet = envelope(magic, size);
    auto receipt = input_latency::parse(packet);
    ASSERT_TRUE(receipt);
    EXPECT_EQ(receipt->input.size(), size);
    EXPECT_EQ(receipt->input.data(), packet.data() + 16);
    for (int i = 0; i < 8; ++i) {
      EXPECT_EQ(receipt->ack[i], 0x80 + i);
    }
    EXPECT_EQ(receipt->ack[8], magic);
    EXPECT_EQ(receipt->ack[9], 0);
    EXPECT_EQ(receipt->ack[10], 0);
    EXPECT_EQ(receipt->ack[11], 0);
  }
}

TEST(InputLatency, RejectsAllTruncatedPackets) {
  for (auto [magic, size] : {std::pair {7, 12}, {5, 18}, {8, 9}, {9, 9}}) {
    auto packet = envelope(magic, size);
    for (std::size_t length = 0; length < packet.size(); ++length) {
      EXPECT_FALSE(input_latency::parse(std::span(packet).first(length)));
    }
  }
}

TEST(InputLatency, RejectsLengthMismatchesUnknownTypesAndNestedEnvelopes) {
  auto original = envelope(7, 12);
  for (auto offset : {3, 4, 7, 19, 20}) {
    auto packet = original;
    packet[offset] ^= 1;
    EXPECT_FALSE(input_latency::parse(packet));
  }
  auto packet = original;
  packet.push_back(0);
  packet[3]++;
  EXPECT_FALSE(input_latency::parse(packet));
  packet = original;
  packet[20] = 0xF0;
  packet[23] = 0x55;
  EXPECT_FALSE(input_latency::parse(packet));
  EXPECT_FALSE(input_latency::parse(envelope(3, 12)));
}

TEST(InputLatency, ReadsBothByteOrdersWithoutAlignmentRequirements) {
  std::array<std::uint8_t, 5> bytes {0, 0x12, 0x34, 0x56, 0x78};
  EXPECT_EQ(input_latency::read32(bytes.data() + 1, true), 0x12345678u);
  EXPECT_EQ(input_latency::read32(bytes.data() + 1, false), 0x78563412u);
}
