
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
#include <shared/intermediate.h>

#define ARRAY_SIZE(a) (sizeof((a)) / sizeof(*(a)))

#define CHUNK_SIZE 2

enum error {
    ERROR_OK = 0,
    ERROR_FAIL = -1,
    ERROR_FILE_TOO_SMALL = -2,
    ERROR_WRONG_MAGIC = -3,
    ERROR_JUNK_AT_END = -4,
    ERROR_INCOMPATIBLE_VERSION = -5,
    ERROR_INCORRECT_STRING_COUNT = -6,
};

static const uint8_t magic[] = {
    0x48, 0x44, 0x4c, 0x49
//  'H',  'D',  'L',  'I'
};

enum error file_read_buffer(FILE *f, uint8_t *buffer, size_t size) {
    if (fread(buffer, 1, size, f) != size) {
        if (feof(f)) {
            log_debug("Reached EOF while reading %lu bytes\n", size);
            return ERROR_FILE_TOO_SMALL;
        }
        return ERROR_FAIL;
    }
    return ERROR_OK;
}

enum error file_read_8(FILE *f, uint8_t *value) {
    uint8_t buffer[1];
    enum error err = file_read_buffer(f, buffer, 1);
    if (err != ERROR_OK) {
        return err;
    }
    *value = buffer[0];
    return ERROR_OK;
}

enum error file_read_16(FILE *f, uint16_t *value) {
    uint8_t buffer[2];
    enum error err = file_read_buffer(f, buffer, 2);
    if (err != ERROR_OK) {
        return err;
    }
    *value = buffer_read16le(buffer, 0);
    return ERROR_OK;
}

enum error file_read_32(FILE *f, uint32_t *value) {
    uint8_t buffer[4];
    enum error err = file_read_buffer(f, buffer, 4);
    if (err != ERROR_OK) {
        return err;
    }
    *value = buffer_read32le(buffer, 0);
    return ERROR_OK;
}

enum error file_write_buffer(FILE *f, const uint8_t *buffer, size_t size) {
    if (fwrite(buffer, 1, size, f) != size) {
        return ERROR_FAIL;
    }
    return ERROR_OK;
}

enum error file_write_8(FILE *f, uint8_t value) {
    return file_write_buffer(f, &value, 1);
}

enum error file_write_16(FILE *f, uint16_t value) {
    uint8_t buffer[2];
    buffer_write16le(buffer, 0, value);
    return file_write_buffer(f, buffer, 2);
}

enum error file_write_32(FILE *f, uint32_t value) {
    uint8_t buffer[4];
    buffer_write32le(buffer, 0, value);
    return file_write_buffer(f, buffer, 4);
}

struct string_table {
    size_t size;
    size_t count;
    char *buffer;
};

uint32_t string_table_add(struct string_table *st, const char *str) {
    size_t len = strlen(str) + 1;
    st->buffer = xrealloc(st->buffer, st->size + len);
    strcpy(st->buffer + st->size, str);
    st->size += len;
    return st->count++;
}

enum error parse_string_table(uint8_t *buffer, uint32_t size, char ***string_table_out, size_t *string_count_out) {
    if (size == 0) {
        *string_table_out = NULL;
        *string_count_out = 0;
        return ERROR_OK;
    }

    uint32_t string_count = 1;
    char **string_table = xmalloc(sizeof(char *) * string_count);
    string_table[0] = (char *) &buffer[0];
    uint32_t pos = 1;

    do {
        if (buffer[pos - 1] == '\0') {
            string_count++;
            string_table = xrealloc(string_table, sizeof(char *) * string_count);
            string_table[string_count - 1] = (char *) &buffer[pos];
        }
        pos++;
    } while (pos < size);

    if (buffer[pos - 1] != '\0') {
        log_debug("Last string not terminated\n");
        xfree(string_table);
        return ERROR_FAIL;
    }

    *string_table_out = string_table;
    *string_count_out = string_count;

    return ERROR_OK;
}

enum error parse_block_statement(FILE *f) {
    enum error err;

    uint16_t op;
    err = file_read_16(f, &op);
    if (err) {
        return err;
    }

    uint16_t size;
    err = file_read_16(f, &size);
    if (err) {
        return err;
    }

    switch (op) {
        case INTERMEDIATE_OP_AND: {
            log_debug("  Statement: op=AND, size=%u\n", size);

            if (size < 2) {
                log_debug("AND can't have %u inputs\n", size);
                return ERROR_FAIL;
            }

            for (uint32_t i = 0; i < size; i++) {
                uint32_t input_id;
                err = file_read_32(f, &input_id);
                if (err) {
                    return err;
                }

                log_debug("    Input: id=%u\n", input_id);
            }

            uint32_t output_id;
            err = file_read_32(f, &output_id);
            if (err) {
                return err;
            }

            log_debug("    Output: id=%u\n", output_id);
            break;
        }
        case INTERMEDIATE_OP_MUX: {
            if (size == 0) {
                log_debug("MUX must have at least 1 address input\n");
                return ERROR_FAIL;
            }

            if (size > 15) {
                log_debug("MUX can't have more than 15 address inputs\n");
                return ERROR_FAIL;
            }

            uint32_t input_count = 1 << size;

            log_debug("  Statement: op=MUX, data inputs=%u, addr inputs=%u\n", input_count, size);

            for (uint32_t i = 0; i < size; i++) {
                uint32_t addr_input_id;
                err = file_read_32(f, &addr_input_id);
                if (err) {
                    return err;
                }

                log_debug("    Address input: id=%u\n", addr_input_id);
            }

            for (uint32_t i = 0; i < input_count; i++) {
                uint32_t input_id;
                err = file_read_32(f, &input_id);
                if (err) {
                    return err;
                }

                log_debug("    Input: id=%u\n", input_id);
            }

            uint32_t output_id;
            err = file_read_32(f, &output_id);
            if (err) {
                return err;
            }

            log_debug("    Output: id=%u\n", output_id);
            break;
        }
        default:
            log_debug("Invalid statement op\n");
            return ERROR_FAIL;
    }

    return ERROR_OK;
}

enum error write_block_statement(FILE *f, struct intermediate_statement *statement) {
    enum error err;

    err = file_write_16(f, statement->op);
    if (err) {
        return err;
    }

    err = file_write_16(f, statement->size);
    if (err) {
        return err;
    }

    for (uint32_t i = 0; i < statement->input_count; i++) {
        err = file_write_32(f, statement->inputs[i]);
        if (err) {
            return err;
        }
    }

    for (uint32_t i = 0; i < statement->output_count; i++) {
        err = file_write_32(f, statement->outputs[i]);
        if (err) {
            return err;
        }
    }

    return ERROR_OK;
}

enum error parse_block(FILE *f) {
    enum error err;

    uint32_t input_count;
    err = file_read_32(f, &input_count);
    if (err) {
        return err;
    }

    uint32_t output_count;
    err = file_read_32(f, &output_count);
    if (err) {
        return err;
    }

    uint32_t statement_count;
    err = file_read_32(f, &statement_count);
    if (err) {
        return err;
    }

    uint32_t name_index;
    err = file_read_32(f, &name_index);
    if (err) {
        return err;
    }

    log_debug("Block: name=%u, inputs=%u, outputs=%u, statements=%u\n", name_index, input_count, output_count, statement_count);

    uint32_t signal_id = 0;

    for (uint32_t i = 0; i < input_count; i++) {
        uint32_t input_name_index;
        err = file_read_32(f, &input_name_index);
        if (err) {
            return err;
        }

        uint16_t input_width;
        err = file_read_16(f, &input_width);
        if (err) {
            return err;
        }

        log_debug("  Input: name=%u, width=%u, id=%u\n", input_name_index, input_width, signal_id);

        signal_id++;
    }

    for (uint32_t i = 0; i < output_count; i++) {
        uint32_t output_name_index;
        err = file_read_32(f, &output_name_index);
        if (err) {
            return err;
        }

        uint16_t output_width;
        err = file_read_16(f, &output_width);
        if (err) {
            return err;
        }

        log_debug("  Output: name=%u, width=%u, id=%u\n", output_name_index, output_width, signal_id);

        signal_id++;
    }

    for (uint32_t i = 0; i < statement_count; i++) {
        err = parse_block_statement(f);
        if (err) {
            return err;
        }
    }

    return ERROR_OK;
}

enum error write_block(FILE *f, struct intermediate_block *block) {
    enum error err;

    err = file_write_32(f, block->input_count);
    if (err) {
        return err;
    }

    err = file_write_32(f, block->output_count);
    if (err) {
        return err;
    }

    err = file_write_32(f, block->statement_count);
    if (err) {
        return err;
    }

    err = file_write_32(f, 42);
    if (err) {
        return err;
    }

    for (uint32_t i = 0; i < block->input_count; i++) {
        struct intermediate_input *input = &block->inputs[i];

        err = file_write_32(f, 42);
        if (err) {
            return err;
        }

        err = file_write_16(f, input->width);
        if (err) {
            return err;
        }
    }

    for (uint32_t i = 0; i < block->output_count; i++) {
        struct intermediate_output *output = &block->outputs[i];

        err = file_write_32(f, 42);
        if (err) {
            return err;
        }

        err = file_write_16(f, output->width);
        if (err) {
            return err;
        }
    }

    for (uint32_t i = 0; i < block->statement_count; i++) {
        err = write_block_statement(f, &block->statements[i]);
        if (err) {
            return err;
        }
    }

    return ERROR_OK;
}

enum error parse_blocks(FILE *f) {
    enum error err;

    uint32_t block_count;
    err = file_read_32(f, &block_count);
    if (err) {
        return err;
    }

    log_debug("%u blocks\n", block_count);

    for (uint32_t i = 0; i < block_count; i++) {
        err = parse_block(f);
        if (err) {
            return err;
        }
    }

    return ERROR_OK;
}

enum error write_blocks(FILE *f, struct intermediate_block **blocks, uint32_t block_count) {
    enum error err;

    err = file_write_32(f, block_count);
    if (err) {
        return err;
    }

    for (uint32_t i = 0; i < block_count; i++) {
        err = write_block(f, blocks[i]);
        if (err) {
            return err;
        }
    }

    return ERROR_OK;
}

enum error parse_file(FILE *f) {
    enum error err;

    // Check magic
    uint8_t file_magic[4];
    err = file_read_buffer(f, file_magic, 4);
    if (err) {
        return err;
    }
    for (size_t i = 0; i < 4; i++) {
        if (file_magic[i] != magic[i]) {
            return ERROR_WRONG_MAGIC;
        }
    }

    // Check version
    uint32_t version;
    err = file_read_32(f, &version);
    if (err) {
        return err;
    }
    log_debug("Version: %u\n", version);
    if (version > INTERMEDIATE_FILE_VERSION) {
        return ERROR_INCOMPATIBLE_VERSION;
    }

    err = parse_blocks(f);
    if (err) {
        return err;
    }

    uint32_t string_table_size;
    err = file_read_32(f, &string_table_size);
    if (err) {
        return err;
    }

    uint8_t *string_table_buffer = xmalloc(string_table_size);
    err = file_read_buffer(f, string_table_buffer, string_table_size);
    if (err) {
        return err;
    }

    size_t string_count;
    char **string_table;
    err = parse_string_table(string_table_buffer, string_table_size, &string_table, &string_count);
    if (err != ERROR_OK) {
        return err;
    }

    for (size_t i = 0; i < string_count; i++) {
        log_debug("String %lu: %s\n", i, string_table[i]);
    }

    if (fgetc(f) != EOF) {
        return ERROR_JUNK_AT_END;
    }

    return ERROR_OK;
}

enum error write_file(FILE *f, struct intermediate_block **blocks, uint32_t block_count) {
    enum error err;

    // Write magic
    err = file_write_buffer(f, magic, 4);
    if (err) {
        return err;
    }

    // Write version
    err = file_write_32(f, INTERMEDIATE_FILE_VERSION);
    if (err) {
        return err;
    }

    struct string_table *st = xcalloc(1, sizeof(struct string_table));
    uint32_t id = string_table_add(st, "peter");
    log_debug("Added \"peter\" as %u\n", id);
    id = string_table_add(st, "horst");
    log_debug("Added \"horst\" as %u\n", id);

    err = write_blocks(f, blocks, block_count);
    if (err) {
        return err;
    }

    err = file_write_32(f, st->size);
    if (err) {
        return err;
    }

    err = file_write_buffer(f, (uint8_t *) st->buffer, st->size);
    if (err) {
        return err;
    }

    return ERROR_OK;
}

int intermediate_file_read(const char *filename) {
    enum error err;

    FILE *f = fopen(filename, "rb");
    if (f == NULL) {
        log_error("Error: Failed to open file: %s\n", strerror(errno));
        return ERROR_FAIL;
    }

    err = parse_file(f);
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
        case ERROR_INCORRECT_STRING_COUNT:
            log_error("Error: Incorrect number of strings\n");
            break;
    }

    fclose(f);

    return err;
}

int intermediate_file_write(const char *filename, struct intermediate_block **blocks, uint32_t block_count) {
    enum error err;

    FILE *f = fopen(filename, "wb");
    if (f == NULL) {
        log_error("Error: Failed to open file: %s\n", strerror(errno));
        return ERROR_FAIL;
    }

    err = write_file(f, blocks, block_count);

    fclose(f);

    return err;
}
