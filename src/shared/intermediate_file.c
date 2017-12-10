
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

uint32_t string_table_add(struct string_table *string_table, const char *str) {
    size_t len = strlen(str) + 1;
    string_table->buffer = xrealloc(string_table->buffer, string_table->size + len);
    strcpy(string_table->buffer + string_table->size, str);
    string_table->size += len;
    return string_table->count++;
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

enum error parse_block_statement(FILE *f, struct intermediate_statement *stmt) {
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

    uint32_t input_count, output_count;

    switch (op) {
        case INTERMEDIATE_OP_CONNECT:
            if (size != 1) {
                log_debug("CONNECT can't have other than 1 input\n");
                return ERROR_FAIL;
            }

            input_count = 1;
            output_count = 1;
            break;
        case INTERMEDIATE_OP_CONST_0:
        case INTERMEDIATE_OP_CONST_1:
            if (size != 1) {
                log_debug("CONST_X can't have other than 0 inputs\n");
                return ERROR_FAIL;
            }

            input_count = 0;
            output_count = 1;
            break;
        case INTERMEDIATE_OP_AND:
            if (size < 2) {
                log_debug("AND can't have %u inputs\n", size);
                return ERROR_FAIL;
            }

            input_count = size;
            output_count = 1;
            break;
        case INTERMEDIATE_OP_OR:
            if (size < 2) {
                log_debug("OR can't have %u inputs\n", size);
                return ERROR_FAIL;
            }

            input_count = size;
            output_count = 1;
            break;
        case INTERMEDIATE_OP_XOR:
            if (size < 2) {
                log_debug("XOR can't have %u inputs\n", size);
                return ERROR_FAIL;
            }

            input_count = size;
            output_count = 1;
            break;
        case INTERMEDIATE_OP_NOT:
            if (size != 1) {
                log_debug("NOT can't have other than 1 input\n");
                return ERROR_FAIL;
            }

            input_count = 1;
            output_count = 1;
            break;
        case INTERMEDIATE_OP_MUX:
            if (size == 0) {
                log_debug("MUX must have at least 1 address input\n");
                return ERROR_FAIL;
            }

            if (size > 15) {
                log_debug("MUX can't have more than 15 address inputs\n");
                return ERROR_FAIL;
            }

            input_count = (1 << size) + size;
            output_count = 1;
            break;
        default:
            log_debug("Invalid statement op\n");
            return ERROR_FAIL;
    }

    stmt->op = op;
    stmt->size = size;
    stmt->input_count = input_count;
    stmt->inputs = stmt->input_count > 0 ? xcalloc(stmt->input_count, sizeof(uint32_t)) : NULL;
    stmt->output_count = output_count;
    stmt->outputs = stmt->output_count > 0 ? xcalloc(stmt->output_count, sizeof(uint32_t)) : NULL;

    for (uint32_t i = 0; i < input_count; i++) {
        uint32_t input_id;
        err = file_read_32(f, &input_id);
        if (err) {
            return err;
        }

        stmt->inputs[i] = input_id;
    }

    for (uint32_t i = 0; i < output_count; i++) {
        uint32_t output_id;
        err = file_read_32(f, &output_id);
        if (err) {
            return err;
        }

        stmt->outputs[i] = output_id;
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

enum error parse_block(FILE *f, struct intermediate_block *block) {
    enum error err;

    uint32_t name_index;
    err = file_read_32(f, &name_index);
    if (err) {
        return err;
    }

    uint32_t input_signals;
    err = file_read_32(f, &input_signals);
    if (err) {
        return err;
    }

    uint32_t output_signals;
    err = file_read_32(f, &output_signals);
    if (err) {
        return err;
    }

    uint32_t block_count;
    err = file_read_32(f, &block_count);
    if (err) {
        return err;
    }

    uint32_t statement_count;
    err = file_read_32(f, &statement_count);
    if (err) {
        return err;
    }

    block->name_index = name_index;
    block->input_signals = input_signals;
    block->output_signals = output_signals;
    block->block_count = block_count;
    block->blocks = block->block_count > 0 ? xcalloc(block->block_count, sizeof(struct intermediate_block *)) : NULL;
    block->statement_count = statement_count;
    block->statements = block->statement_count > 0 ? xcalloc(block->statement_count, sizeof(struct intermediate_statement)) : NULL;

    block->next_signal = block->input_signals + block->output_signals;

    // TODO: blocks

    for (uint32_t i = 0; i < statement_count; i++) {
        err = parse_block_statement(f, &block->statements[i]);
        if (err) {
            return err;
        }
    }

    return ERROR_OK;
}

enum error write_block(FILE *f, struct intermediate_block *block, struct string_table *string_table) {
    enum error err;

    err = file_write_32(f, string_table_add(string_table, block->name));
    if (err) {
        return err;
    }

    err = file_write_32(f, block->input_signals);
    if (err) {
        return err;
    }

    err = file_write_32(f, block->output_signals);
    if (err) {
        return err;
    }

    err = file_write_32(f, block->block_count);
    if (err) {
        return err;
    }

    err = file_write_32(f, block->statement_count);
    if (err) {
        return err;
    }

    // TODO: blocks

    for (uint32_t i = 0; i < block->statement_count; i++) {
        err = write_block_statement(f, &block->statements[i]);
        if (err) {
            return err;
        }
    }

    return ERROR_OK;
}

enum error parse_blocks(FILE *f, struct intermediate_block ***blocks_out, uint32_t *block_count_out) {
    enum error err;

    uint32_t block_count;
    err = file_read_32(f, &block_count);
    if (err) {
        return err;
    }

    struct intermediate_block **blocks = xcalloc(block_count, sizeof(struct intermediate_block *));

    for (uint32_t i = 0; i < block_count; i++) {
        struct intermediate_block *block = xcalloc(1, sizeof(struct intermediate_block));

        err = parse_block(f, block);
        if (err) {
            return err;
        }

        blocks[i] = block;
    }

    *blocks_out = blocks;
    *block_count_out = block_count;

    return ERROR_OK;
}

enum error write_blocks(FILE *f, struct intermediate_block **blocks, uint32_t block_count, struct string_table *string_table) {
    enum error err;

    err = file_write_32(f, block_count);
    if (err) {
        return err;
    }

    for (uint32_t i = 0; i < block_count; i++) {
        err = write_block(f, blocks[i], string_table);
        if (err) {
            return err;
        }
    }

    return ERROR_OK;
}

enum error parse_file(FILE *f, struct intermediate_file *file) {
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

    if (version > INTERMEDIATE_FILE_VERSION) {
        return ERROR_INCOMPATIBLE_VERSION;
    }

    struct intermediate_block **blocks;
    uint32_t block_count;

    err = parse_blocks(f, &blocks, &block_count);
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

    for (uint32_t i = 0; i < block_count; i++) {
        struct intermediate_block *block = blocks[i];

        if (block->name_index >= string_count) {
            return ERROR_FAIL;
        }

        block->name = string_table[block->name_index];
    }

    if (fgetc(f) != EOF) {
        return ERROR_JUNK_AT_END;
    }

    file->blocks = blocks;
    file->block_count = block_count;

    return ERROR_OK;
}

enum error write_file(FILE *f, struct intermediate_file *file) {
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

    struct string_table *string_table = xcalloc(1, sizeof(struct string_table));

    err = write_blocks(f, file->blocks, file->block_count, string_table);
    if (err) {
        return err;
    }

    err = file_write_32(f, string_table->size);
    if (err) {
        return err;
    }

    err = file_write_buffer(f, (uint8_t *) string_table->buffer, string_table->size);
    if (err) {
        return err;
    }

    xfree(string_table);

    return ERROR_OK;
}

int intermediate_file_read(const char *filename, struct intermediate_file *file) {
    enum error err;

    FILE *f = fopen(filename, "rb");
    if (f == NULL) {
        log_error("Error: Failed to open file: %s\n", strerror(errno));
        return ERROR_FAIL;
    }

    err = parse_file(f, file);
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

int intermediate_file_write(const char *filename, struct intermediate_file *file) {
    enum error err;

    FILE *f = fopen(filename, "wb");
    if (f == NULL) {
        log_error("Error: Failed to open file: %s\n", strerror(errno));
        return ERROR_FAIL;
    }

    err = write_file(f, file);

    fclose(f);

    return err;
}
