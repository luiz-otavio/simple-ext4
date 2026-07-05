#pragma once

#include <cstdint>
#include "context.h"

// Consulta se um inode está ocupado (1) ou livre (0).
// inode_number é 1-based (inode -> 1 = primeiro inode, 2 = segundo inode, etc.)
int ext4_is_inode_used(const ext4_context* ctx, uint32_t inode_number);

// Consulta se um bloco está ocupado (1) ou livre (0).
// block_number é 1-based (block -> 1 = primeiro bloco, 2 = segundo bloco, etc.)
int ext4_is_block_used(const ext4_context* ctx, uint32_t block_number);

// Encontra e marca como usado o primeiro inode livre.
// OBS: Esse método ja transforma o inode_number para o grupo de blocos correto e persiste a alteração no bitmap de inodes na imagem.
// Escreve o número do inode em *out_inode_number.
// Retorna 0 em sucesso, -1 se não houver inode livre.
int ext4_alloc_inode(ext4_context* ctx, uint32_t* out_inode_number);

// Encontra e marca como usado o primeiro bloco livre.
// OBS: Esse método ja transforma o block_number para o grupo de blocos correto e persiste a alteração no bitmap de blocos na imagem.
// Escreve o número do bloco em *out_block_number.
// Retorna 0 em sucesso, -1 se não houver bloco livre.
int ext4_alloc_block(ext4_context* ctx, uint32_t* out_block_number);

// Marca o inode como livre no bitmap e persiste a alteração na imagem.
// Retorna 0 em sucesso, -1 em erro.
int ext4_free_inode(ext4_context* ctx, uint32_t inode_number);

// Marca o bloco como livre no bitmap e persiste a alteração na imagem.
// Retorna 0 em sucesso, -1 em erro.
int ext4_free_block(ext4_context* ctx, uint32_t block_number);
