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
    uint16_t i_mode;         
    uint16_t i_uid;         
    uint32_t i_size_lo;     
    uint32_t i_atime;        
    uint32_t i_ctime;        
    uint32_t i_mtime;        
    uint32_t i_dtime;        
    uint16_t i_gid;          
    uint16_t i_links_count; 
    uint32_t i_blocks_lo;   // Número de blocos
    uint32_t i_flags;        
    uint32_t i_osd1;         
    char     i_block[60];    /* 0x28: blocos/extents */
    uint32_t i_generation;   
    uint32_t i_file_acl_lo;  
    uint32_t i_size_high;    
    uint32_t i_obso_faddr;   
    uint8_t  i_osd2[12];     
};

struct ext4_dir_entry {
    uint32_t inode; // Inode number do arquivo/diretório apontado por esta entrada
    uint16_t rec_len; // Tamanho desta entrada de diretório em bytes (mínimo 8 + name_len, alinhado a 4 bytes)
    uint8_t  name_len; // Comprimento do nome do arquivo/diretório
    uint8_t  file_type; // Tipo do arquivo (EXT4_FT_REG_FILE = arquivo regular, EXT4_FT_DIR = diretório, etc)
    char     name[255]; // Nome do arquivo/diretório (não fixo, mas o tamanho máximo é 255 bytes)
};

struct ext4_extent_header {
    uint16_t eh_magic; // Magic number do header de extents (0xF30A)
    uint16_t eh_entries; // Número de entradas (ext4_extent ou ext4_extent_idx) no bloco
    uint16_t eh_max; // Número máximo de entradas que cabem no bloco
    uint16_t eh_depth; // 0 = bloco de dados, >0 = bloco de índices 
    uint32_t not_used_eh_generation; // Número de versão do extent tree (não usado nesse projeto)
};

struct ext4_extent_idx {
    uint32_t ei_block; // Primeiro bloco lógico do extent apontado por este índice
    uint32_t ei_leaf_lo; // Bloco físico do extent apontado por este índice (parte baixa)
    uint16_t ei_leaf_hi; // Bloco físico do extent apontado por este índice (parte alta)
    uint16_t ei_unused;
};

struct ext4_extent {
    uint32_t ee_block; // Primeiro bloco lógico do extent
    // Número de blocos do extent
    // Se ee_len > 1, significa que o extent cobre blocos consecutivos (por isso ee_len * block_size = tamanho em bytes do extent)
    // Se ee_len > 32768, significa que o extent cobre blocos não consecutivos (por isso ee_len - 32768 = número de blocos não consecutivos do extent)
    uint16_t ee_len;
    uint16_t ee_start_hi; // Parte alta do bloco físico do extent
    uint32_t ee_start_lo; // Parte baixa do bloco físico do extent
};
