#pragma once

#include <cstddef>
#include <cstdint>

#ifndef __nonstring
#define __nonstring
#endif

#ifndef EXT4_INODE_TABLE_BLOCKS
#define EXT4_INODE_TABLE_BLOCKS 15
#endif

#define EXT4_ROOT_INO        2 // Número do inode da raiz do sistema de arquivos EXT4
#define EXT4_EXTENTS_FL      0x00080000 // Flag do inode indicando que o arquivo/diretório usa extents (necessário para esse projeto) (também checa no superbloco se o filesystem tem suporte a extents)
#define EXT4_EXTENT_MAGIC    0xF30A // Magic number do header de extents (necessário para esse projeto) (também checa no superbloco se o filesystem tem suporte a extents)
#define EXT4_FT_UNKNOWN      0
#define EXT4_FT_REG_FILE     1 // Se o campo type do ext4_dir_entry for 1, significa que é um arquivo regular
#define EXT4_FT_DIR          2 // Se o campo type do ext4_dir_entry for 2, significa que é um diretório

struct ext4_superblock {
    uint32_t s_inodes_count;        /* Total de inodes */
    uint32_t s_blocks_count_lo;     /* Total de blocos */
    uint32_t s_free_blocks_count_lo;
    uint32_t s_free_inodes_count;
    uint32_t s_log_block_size;      /* log2(block_size) - 10 */
    uint32_t s_blocks_per_group;
    uint32_t s_inodes_per_group;
    uint16_t s_magic;               /* 0xEF53 */
    uint16_t s_inode_size;          /* Tamanho do inode em bytes */
    uint32_t s_feature_incompat;    /* bit 0x40 = extents habilitado */
    uint32_t s_desc_size;           /* Tamanho do block group descriptor */
    char     s_volume_name[16];     /* Nome do volume */
};

struct ext4_block_group_descriptor {
    uint32_t bg_block_bitmap_lo;  /* Bloco do block bitmap deste grupo */
    uint32_t bg_inode_bitmap_lo;  /* Bloco do inode bitmap deste grupo */
    uint32_t bg_inode_table_lo;   /* Bloco inicial da inode table deste grupo */
};

struct ext4_inode {
    uint16_t i_mode;         /* 0x00: Tipo e permissões */
    uint16_t i_uid;          /* 0x02: UID do dono */
    uint32_t i_size_lo;      /* 0x04: Tamanho baixo */
    uint32_t i_atime;        /* 0x08 */
    uint32_t i_ctime;        /* 0x0C */
    uint32_t i_mtime;        /* 0x10 */
    uint32_t i_dtime;        /* 0x14 */
    uint16_t i_gid;          /* 0x18 */
    uint16_t i_links_count;  /* 0x1A */
    uint32_t i_blocks_lo;    /* 0x1C: em setores de 512 bytes */
    uint32_t i_flags;        /* 0x20 */
    uint32_t i_osd1;         /* 0x24 */
    char     i_block[60];    /* 0x28: blocos/extents */
    uint32_t i_generation;   /* 0x64 */
    uint32_t i_file_acl_lo;  /* 0x68 */
    uint32_t i_size_high;    /* 0x6C */
    uint32_t i_obso_faddr;   /* 0x70 */
    uint8_t  i_osd2[12];     /* 0x74 */
};

struct ext4_dir_entry {
    uint32_t inode;
    uint16_t rec_len;
    uint8_t  name_len;
    uint8_t  file_type;
    char     name[255];
};

struct ext4_extent_header {
    uint16_t eh_magic;
    uint16_t eh_entries;
    uint16_t eh_max;
    uint16_t eh_depth;
    uint32_t eh_generation;
};

struct ext4_extent_idx {
    uint32_t ei_block;
    uint32_t ei_leaf_lo;
    uint16_t ei_leaf_hi;
    uint16_t ei_unused;
};

struct ext4_extent {
    uint32_t ee_block;
    uint16_t ee_len;
    uint16_t ee_start_hi;
    uint32_t ee_start_lo;
};
