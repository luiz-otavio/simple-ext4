#pragma once

#include "context.h"

// Lê o inode de número 'inode_number' no contexto dado.
// Esse método já transforma o inode_number para o grupo de blocos correto e lê o inode da imagem.
// Retorna 0 em sucesso, negativo em erro.
int ext4_get_inode(const ext4_context* ctx, uint32_t inode_number, ext4_inode* out_inode);

// Lê todos os dados de um inode (via extents depth=0) em buffer alocado pelo chamador.
// *out_size recebe o total de bytes lidos.
// O buffer retornado em *out_data deve ser liberado com delete[].
// Retorna 0 em sucesso, negativo em erro.
int ext4_read_inode_data(const ext4_context* ctx, const ext4_inode* inode,
                         char** out_data, uint32_t* out_size);

// Escreve o inode 'inode_number' de volta na imagem.
// Esse método já transforma o inode_number para o grupo de blocos correto e persiste a alteração na imagem.
// Retorna 0 em sucesso, negativo em erro.
int ext4_write_inode(const ext4_context* ctx, uint32_t inode_number, const ext4_inode* inode);

// Aloca um bloco de dados e inicializa com zeros na imagem.
// Retorna o número do bloco ou 0 em erro.
uint32_t ext4_alloc_and_zero_block(ext4_context* ctx);
