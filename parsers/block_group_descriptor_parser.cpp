#include <cstddef>
#include <cstdint>
#include <cstring>

#include "block_group_descriptor_parser.h"

int parse_block_group_descriptor(const char* block, struct ext4_block_group_descriptor* bgd) {
    if (block == nullptr || bgd == nullptr) {
        return -1;
    }

    // Copia da memoria | index | tamanho
    memcpy(&bgd->bg_block_bitmap_lo, block + 0x00, sizeof(uint32_t));
    memcpy(&bgd->bg_inode_bitmap_lo, block + 0x04, sizeof(uint32_t));
    memcpy(&bgd->bg_inode_table_lo,  block + 0x08, sizeof(uint32_t));

    return 0;
}
