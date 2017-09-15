
#pragma once

#include <stdint.h>

static inline uint8_t buffer_read8(uint8_t *buffer, size_t address) {
    return (uint8_t) buffer[address + 0] << 0;
}

static inline uint16_t buffer_read16le(uint8_t *buffer, size_t address) {
    return (uint16_t) buffer[address + 0] << 0
         | (uint16_t) buffer[address + 1] << 8;
}

static inline uint16_t buffer_read16be(uint8_t *buffer, size_t address) {
    return (uint16_t) buffer[address + 0] << 8
         | (uint16_t) buffer[address + 1] << 0;
}

static inline uint32_t buffer_read32le(uint8_t *buffer, size_t address) {
    return (uint32_t) buffer[address + 0] << 0
         | (uint32_t) buffer[address + 1] << 8
         | (uint32_t) buffer[address + 2] << 16
         | (uint32_t) buffer[address + 3] << 24;
}

static inline uint32_t buffer_read32be(uint8_t *buffer, size_t address) {
    return (uint32_t) buffer[address + 0] << 24
         | (uint32_t) buffer[address + 1] << 16
         | (uint32_t) buffer[address + 2] << 8
         | (uint32_t) buffer[address + 3] << 0;
}

static inline uint64_t buffer_read64le(uint8_t *buffer, size_t address) {
    return (uint64_t) buffer[address + 0] << 0
         | (uint64_t) buffer[address + 1] << 8
         | (uint64_t) buffer[address + 2] << 16
         | (uint64_t) buffer[address + 3] << 24
         | (uint64_t) buffer[address + 4] << 32
         | (uint64_t) buffer[address + 5] << 40
         | (uint64_t) buffer[address + 6] << 48
         | (uint64_t) buffer[address + 7] << 56;
}

static inline uint64_t buffer_read64be(uint8_t *buffer, size_t address) {
    return (uint64_t) buffer[address + 0] << 56
         | (uint64_t) buffer[address + 1] << 48
         | (uint64_t) buffer[address + 2] << 40
         | (uint64_t) buffer[address + 3] << 32
         | (uint64_t) buffer[address + 4] << 24
         | (uint64_t) buffer[address + 5] << 16
         | (uint64_t) buffer[address + 6] << 8
         | (uint64_t) buffer[address + 7] << 0;
}
