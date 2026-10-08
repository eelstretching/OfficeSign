#include "WireFormat.h"

#include <string.h>

uint8_t ByteReader::readU8() {
    if (!hasRemaining(1)) {
        failed = true;
        return 0;
    }
    return data[pos++];
}

uint16_t ByteReader::readU16LE() {
    if (!hasRemaining(2)) {
        failed = true;
        return 0;
    }
    uint16_t v = (uint16_t)data[pos] | ((uint16_t)data[pos + 1] << 8);
    pos += 2;
    return v;
}

int16_t ByteReader::readI16LE() { return (int16_t)readU16LE(); }

int64_t ByteReader::readI64LE() {
    if (!hasRemaining(8)) {
        failed = true;
        return 0;
    }
    uint64_t v = 0;
    for (int i = 0; i < 8; i++) {
        v |= (uint64_t)data[pos + i] << (8 * i);
    }
    pos += 8;
    return (int64_t)v;
}

void ByteReader::readBytes(char* dst, uint16_t n) {
    if (!hasRemaining(n)) {
        failed = true;
        memset(dst, 0, n);
        return;
    }
    memcpy(dst, data + pos, n);
    pos += n;
}
