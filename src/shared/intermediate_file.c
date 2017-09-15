
#include "intermediate_file.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include <shared/helper/buffer.h>
#include <shared/helper/log.h>
#include <shared/helper/mem.h>

#define ARRAY_SIZE(a) (sizeof((a)) / sizeof(*(a)))

#define CHUNK_SIZE 2

enum error {
    ERROR_OK = 0,
    ERROR_FAIL = -1,
    ERROR_FILE_TOO_SMALL = -2,
    ERROR_WRONG_MAGIC = -3,
    ERROR_JUNK_AT_END = -4,
    ERROR_INCOMPATIBLE_VERSION = -5,
};

static const uint8_t magic[] = {
    0x48, 0x44, 0x4c, 0x49
//  'H',  'D',  'L',  'I'
};

enum error read_binary_file(const char *filename, uint8_t **buffer_out, size_t *size_out) {
    FILE *f = fopen(filename, "rb");
    if (f == NULL) {
        log_error("Error: Failed to open file %s: %s\n", filename, strerror(errno));
        return -1;
    }

    size_t size = 0;
    size_t capacity = 1;
    uint8_t *buffer = xmalloc(sizeof(uint8_t) * capacity);
    size_t bytes_read;
    do {
        size_t needed_capacity = size + CHUNK_SIZE;

        if (capacity < needed_capacity) {
            size_t new_capacity = capacity;
            do {
                new_capacity *= 2;
            } while (new_capacity < needed_capacity);

            uint8_t *new_buffer = xrealloc(buffer, sizeof(uint8_t) * new_capacity);
            buffer = new_buffer;
            capacity = new_capacity;
        }

        bytes_read = fread(buffer + size, 1, CHUNK_SIZE, f);
        size += bytes_read;
    } while (bytes_read == CHUNK_SIZE);

    if (ferror(f)) {
        xfree(buffer);
        fclose(f);
        return -1;
    }

    fclose(f);

    buffer = xrealloc(buffer, sizeof(uint8_t) * size);

    *buffer_out = buffer;
    *size_out = size;

    return 0;
}

enum error parse_buffer(uint8_t *buffer, size_t size) {
    size_t pos = 0;

    for (size_t i = 0; i < ARRAY_SIZE(magic); i++) {
        if (i >= size) {
            return ERROR_FILE_TOO_SMALL;
        }
        if (buffer[i] != magic[i]) {
            return ERROR_WRONG_MAGIC;
        }
        pos++;
    }

    if (size < pos + 4) {
        return ERROR_FILE_TOO_SMALL;
    }

    uint32_t version = buffer_read32le(buffer, pos);
    pos += 4;

    log_debug("Version: %u\n", version);

    if (version > INTERMEDIATE_FILE_VERSION) {
        return ERROR_INCOMPATIBLE_VERSION;
    }

    if (size < pos + 8) {
        return ERROR_FILE_TOO_SMALL;
    }

    log_debug(" 8: %02x\n", buffer[pos]);
    log_debug("16: %04x\n", buffer_read16le(buffer, pos));
    log_debug("32: %08x\n", buffer_read32le(buffer, pos));
    log_debug("64: %016lx\n", buffer_read64le(buffer, pos));

    pos += 8;

    if (pos != size) {
        return ERROR_JUNK_AT_END;
    }

    return ERROR_OK;
}

int intermediate_file_read(const char *filename) {
    enum error err;

    uint8_t *buffer;
    size_t buffer_size;
    err = read_binary_file(filename, &buffer, &buffer_size);
    if (err) {
        return err;
    }

    for (size_t i = 0; i < buffer_size; i += 16) {
        for (size_t j = 0; j < 16; j++) {
            if (i + j < buffer_size) {
                log_debug("%02x", buffer[i + j]);
            }

            if (i + j < buffer_size - 1) {
                log_debug(" ");
            }
        }
        log_debug("\n");
    }

    err = parse_buffer(buffer, buffer_size);
    switch (err) {
        case ERROR_OK:
            break;
        case ERROR_FAIL:
            log_error("Error: Failure\n");
            break;
        case ERROR_FILE_TOO_SMALL:
            log_error("Error: File too small\n");
            break;
        case ERROR_WRONG_MAGIC:
            log_error("Error: Wrong magic\n");
            break;
        case ERROR_JUNK_AT_END:
            log_error("Error: Junk at end of file\n");
            break;
        case ERROR_INCOMPATIBLE_VERSION:
            log_error("Error: Incompatible intermediate file version\n");
            break;
    }

    xfree(buffer);

    return err;
}