#include <cstddef>
#include <cstdint>
#include <cstring>

#include "dir_entry_parser.h"

int parse_dir_entry(const char* block, uint32_t blockSize, struct ext4_dir_entry* dir_entry) {
    if (block == nullptr || dir_entry == nullptr) {
        return -1;
    }

    uint32_t offset = 0;
    // Eu não sei se esse 8 é uma garantia de que apenas os 3~4 primeiros campos existam.
    // É o que me faz logico, porque o guia da Oracle não me cita o motivo de existir esse 8 bytes necessários
    while (offset + 8 <= blockSize) {
        const char* ptr = block + offset;
        memcpy(&dir_entry->inode,     ptr + 0x00, sizeof(uint32_t));
        memcpy(&dir_entry->rec_len,   ptr + 0x04, sizeof(uint16_t));
        memcpy(&dir_entry->name_len,  ptr + 0x06, sizeof(uint8_t));
        memcpy(&dir_entry->file_type, ptr + 0x07, sizeof(uint8_t));

        if (dir_entry->rec_len < 8 || offset + dir_entry->rec_len > blockSize) {
            return -2;
        }

        if (dir_entry->inode != 0 && dir_entry->name_len > 0) {
            uint8_t copy_len = dir_entry->name_len;
            if (copy_len > sizeof(dir_entry->name) - 1) {
                copy_len = sizeof(dir_entry->name) - 1;
            }
            memcpy(dir_entry->name, ptr + 8, copy_len);
            dir_entry->name[copy_len] = '\0';
            dir_entry->name_len = copy_len;
            return 0;
        } else {
            dir_entry->name[0] = '\0';
        }

        offset += dir_entry->rec_len;
    }

    return -3;
}
