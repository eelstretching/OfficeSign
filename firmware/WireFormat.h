#ifndef WIREFORMAT_H
#define WIREFORMAT_H

#pragma once

#include <stdint.h>

/// @brief Bounds-checked little-endian byte reader for decoding messages
/// received over BLE. Every read is clamped against the buffer length that
/// was passed to the constructor, so a truncated or malformed write can never
/// read out of bounds; reads past the end simply return 0/false and leave the
/// reader in a failed state.
class ByteReader {
   public:
    ByteReader(const uint8_t* data, uint16_t len) : data(data), len(len) {}

    /// @brief Whether at least n more bytes are available to read.
    bool hasRemaining(uint16_t n) const { return !failed && (uint32_t)pos + n <= len; }

    /// @brief Whether a previous read has already run past the end of the buffer.
    bool hasFailed() const { return failed; }

    uint8_t readU8();
    uint16_t readU16LE();
    int16_t readI16LE();
    int64_t readI64LE();

    /// @brief Copies n raw bytes into dst. Does not NUL-terminate; the caller
    /// is responsible for that if dst is meant to be used as a C string.
    void readBytes(char* dst, uint16_t n);

   private:
    const uint8_t* data;
    uint16_t len;
    uint16_t pos = 0;
    bool failed = false;
};

#endif
