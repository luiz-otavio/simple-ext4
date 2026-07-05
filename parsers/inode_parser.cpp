#include <cstddef>
#include <cstdint>
#include <cstring>

#include "inode_parser.h"

int parse_inode(const char* block, struct ext4_inode* inode) {
    if (block == nullptr || inode == nullptr) {
        return -1;
    }

    // Copia da memoria | index | tamanho
    memcpy(&inode->i_mode,        block + 0x00, sizeof(uint16_t));
    memcpy(&inode->i_uid,         block + 0x02, sizeof(uint16_t));
    memcpy(&inode->i_size_lo,     block + 0x04, sizeof(uint32_t));
    memcpy(&inode->i_atime,       block + 0x08, sizeof(uint32_t));
    memcpy(&inode->i_ctime,       block + 0x0C, sizeof(uint32_t));
    memcpy(&inode->i_mtime,       block + 0x10, sizeof(uint32_t));
    memcpy(&inode->i_dtime,       block + 0x14, sizeof(uint32_t));
    memcpy(&inode->i_gid,         block + 0x18, sizeof(uint16_t));
    memcpy(&inode->i_links_count, block + 0x1A, sizeof(uint16_t));
    memcpy(&inode->i_blocks_lo,   block + 0x1C, sizeof(uint32_t));
    memcpy(&inode->i_flags,       block + 0x20, sizeof(uint32_t));
    memcpy(&inode->i_osd1,        block + 0x24, sizeof(uint32_t));
    memcpy(&inode->i_block,       block + 0x28, 60); // Forçando a cópia de 60 bytes para i_block (15 * 4 bytes) (extents habilitado)
    memcpy(&inode->i_generation,  block + 0x64, sizeof(uint32_t));
    memcpy(&inode->i_file_acl_lo, block + 0x68, sizeof(uint32_t));
    memcpy(&inode->i_size_high,   block + 0x6C, sizeof(uint32_t));
    memcpy(&inode->i_obso_faddr,  block + 0x70, sizeof(uint32_t));
    memcpy(&inode->i_osd2,        block + 0x74, 12);

    return 0;
}
