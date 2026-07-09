#pragma once

#include "types.h"

// Estado completo do filesystem carregado em memória.
// Preciso disso para tentar simular como se fosse um sistema de arquivos real
struct ext4_context {
    int      fd;                              // Descritor do arquivo da imagem

    ext4_superblock sb; // Superbloco do contexto
    uint32_t block_size; // Tamanho do bloco em bytes
    uint16_t descriptor_size; // Tamanho do descritor de bloco em bytes
    int      num_groups; // Número de grupos de blocos

    ext4_block_group_descriptor* bgds;        // Array de descritores de grupo
    char**   block_bitmaps;                   // blockBitmaps[grupo]: bitmap de blocos do grupo com N bytes, onde N = sb.s_blocks_per_group / 8
    char**   inode_bitmaps;                   // inodeBitmaps[grupo]: bitmap de inodes do grupo com N bytes, onde N = sb.s_inodes_per_group / 8

    uint32_t current_inode_number;            // Inode do diretório corrente
    char     current_path[4096];              // Caminho absoluto corrente
};

// Inicializa o contexto a partir de um arquivo de imagem.
// Retorna 0 em sucesso, negativo em erro.
// OBS: O contexto só inicia se a imagem for um EXT4 válido (magic number) e com feature de extents habilitada (necessário para esse projeto).
int  ext4_context_init(ext4_context* ctx, const char* image_path);

// Salva o contexto de volta na imagem do arquivo.
// Retorna 0 em sucesso, negativo em erro.
int ext4_context_save(ext4_context* ctx);

// Libera todos os recursos alocados pelo contexto.
void ext4_context_destroy(ext4_context* ctx);
