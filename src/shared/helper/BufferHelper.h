
#pragma once

#include <cstdint>

class BufferHelper {
public:
    static std::uint8_t read8(const std::uint8_t *buffer, std::size_t offset) {
        return (std::uint8_t) buffer[offset + 0];
    }

    static std::uint16_t read16le(const std::uint8_t *buffer, std::size_t offset) {
        return (std::uint16_t) buffer[offset + 0] << 0
               | (std::uint16_t) buffer[offset + 1] << 8;
    }

    static std::uint16_t read16be(const std::uint8_t *buffer, std::size_t offset) {
        return (std::uint16_t) buffer[offset + 0] << 8
               | (std::uint16_t) buffer[offset + 1] << 0;
    }

    static std::uint32_t read32le(const std::uint8_t *buffer, std::size_t offset) {
        return (std::uint32_t) buffer[offset + 0] << 0
               | (std::uint32_t) buffer[offset + 1] << 8
               | (std::uint32_t) buffer[offset + 2] << 16
               | (std::uint32_t) buffer[offset + 3] << 24;
    }

    static std::uint32_t read32be(const std::uint8_t *buffer, std::size_t offset) {
        return (std::uint32_t) buffer[offset + 0] << 24
               | (std::uint32_t) buffer[offset + 1] << 16
               | (std::uint32_t) buffer[offset + 2] << 8
               | (std::uint32_t) buffer[offset + 3] << 0;
    }

    static std::uint64_t read64le(const std::uint8_t *buffer, std::size_t offset) {
        return (std::uint64_t) buffer[offset + 0] << 0
               | (std::uint64_t) buffer[offset + 1] << 8
               | (std::uint64_t) buffer[offset + 2] << 16
               | (std::uint64_t) buffer[offset + 3] << 24
               | (std::uint64_t) buffer[offset + 4] << 32
               | (std::uint64_t) buffer[offset + 5] << 40
               | (std::uint64_t) buffer[offset + 6] << 48
               | (std::uint64_t) buffer[offset + 7] << 56;
    }

    static std::uint64_t read64be(const std::uint8_t *buffer, std::size_t offset) {
        return (std::uint64_t) buffer[offset + 0] << 56
               | (std::uint64_t) buffer[offset + 1] << 48
               | (std::uint64_t) buffer[offset + 2] << 40
               | (std::uint64_t) buffer[offset + 3] << 32
               | (std::uint64_t) buffer[offset + 4] << 24
               | (std::uint64_t) buffer[offset + 5] << 16
               | (std::uint64_t) buffer[offset + 6] << 8
               | (std::uint64_t) buffer[offset + 7] << 0;
    }

    static void write8(std::uint8_t *buffer, std::size_t offset, std::uint8_t value) {
        buffer[offset + 0] = (std::uint8_t) (value >> 0) & (std::uint8_t) 0xff;
    }

    static void write16le(std::uint8_t *buffer, std::size_t offset, std::uint16_t value) {
        buffer[offset + 0] = (std::uint8_t) (value >> 0) & (std::uint8_t) 0xff;
        buffer[offset + 1] = (std::uint8_t) (value >> 8) & (std::uint8_t) 0xff;
    }

    static void write32le(std::uint8_t *buffer, std::size_t offset, std::uint32_t value) {
        buffer[offset + 0] = (std::uint8_t) (value >> 0) & (std::uint8_t) 0xff;
        buffer[offset + 1] = (std::uint8_t) (value >> 8) & (std::uint8_t) 0xff;
        buffer[offset + 2] = (std::uint8_t) (value >> 16) & (std::uint8_t) 0xff;
        buffer[offset + 3] = (std::uint8_t) (value >> 24) & (std::uint8_t) 0xff;
    }

    static void write64le(std::uint8_t *buffer, std::size_t offset, std::uint64_t value) {
        buffer[offset + 0] = (std::uint8_t) (value >> 0) & (std::uint8_t) 0xff;
        buffer[offset + 1] = (std::uint8_t) (value >> 8) & (std::uint8_t) 0xff;
        buffer[offset + 2] = (std::uint8_t) (value >> 16) & (std::uint8_t) 0xff;
        buffer[offset + 3] = (std::uint8_t) (value >> 24) & (std::uint8_t) 0xff;
        buffer[offset + 4] = (std::uint8_t) (value >> 32) & (std::uint8_t) 0xff;
        buffer[offset + 5] = (std::uint8_t) (value >> 40) & (std::uint8_t) 0xff;
        buffer[offset + 6] = (std::uint8_t) (value >> 48) & (std::uint8_t) 0xff;
        buffer[offset + 7] = (std::uint8_t) (value >> 56) & (std::uint8_t) 0xff;
    }
};
