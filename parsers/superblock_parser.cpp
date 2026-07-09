#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>

#include "superblock_parser.h"

#define EXT4_SUPER_MAGIC 0xEF53

int parse_superblock(const char* block, struct ext4_superblock* superblock) {
    if (block == nullptr || superblock == nullptr) {
        return -1;
    }

    // Copia da memoria | index | tamanho
    memcpy(&superblock->s_inodes_count,         block + 0x00, sizeof(uint32_t));
    memcpy(&superblock->s_blocks_count_lo,      block + 0x04, sizeof(uint32_t));
    memcpy(&superblock->s_free_blocks_count_lo, block + 0x0C, sizeof(uint32_t));
    memcpy(&superblock->s_magic, block + 0x38, sizeof(uint16_t)); // A Oracle diz que o magic number é 0xEF53, então vamos checar se é isso mesmo
    memcpy(&superblock->s_free_inodes_count,    block + 0x10, sizeof(uint32_t));
    memcpy(&superblock->s_log_block_size,       block + 0x18, sizeof(uint32_t));
    memcpy(&superblock->s_blocks_per_group,     block + 0x20, sizeof(uint32_t));
    memcpy(&superblock->s_inodes_per_group,     block + 0x28, sizeof(uint32_t));
    memcpy(&superblock->s_inode_size,           block + 0x58, sizeof(uint16_t));
    memcpy(&superblock->s_feature_incompat,     block + 0x60, sizeof(uint32_t));
    memcpy(&superblock->s_volume_name,          block + 0x78, 16);
    memcpy(&superblock->s_desc_size,            block + 0xFE, sizeof(uint16_t));

    return 0;
}

int write_superblock(const struct ext4_superblock* superblock, char* block) {
    if (superblock == nullptr || block == nullptr) {
        return -1;
    }

    // Copia da memoria | index | tamanho
    memcpy(block + 0x00, &superblock->s_inodes_count,         sizeof(uint32_t));
    memcpy(block + 0x04, &superblock->s_blocks_count_lo,      sizeof(uint32_t));
    memcpy(block + 0x0C, &superblock->s_free_blocks_count_lo, sizeof(uint32_t));
    memcpy(block + 0x38, &superblock->s_magic,                sizeof(uint16_t));
    memcpy(block + 0x10, &superblock->s_free_inodes_count,    sizeof(uint32_t));
    memcpy(block + 0x18, &superblock->s_log_block_size,       sizeof(uint32_t));
    memcpy(block + 0x20, &superblock->s_blocks_per_group,     sizeof(uint32_t));
    memcpy(block + 0x28, &superblock->s_inodes_per_group,     sizeof(uint32_t));
    memcpy(block + 0x58, &superblock->s_inode_size,           sizeof(uint16_t));
    memcpy(block + 0x60, &superblock->s_feature_incompat,     sizeof(uint32_t));
    memcpy(block + 0x78, &superblock->s_volume_name,          16);
    memcpy(block + 0xFE, &superblock->s_desc_size,            sizeof(uint16_t));

    return 0;
}