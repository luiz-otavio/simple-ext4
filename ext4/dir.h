#pragma once

#include <cstdint>
#include "context.h"

// Itera todas as dir entries válidas de um diretório.
// Para cada entrada, chama callback(entry, userdata).
// Se callback retornar != 0, a iteração para.
typedef int (*dir_entry_callback)(const ext4_dir_entry* entry, void* userdata);

int ext4_list_dir(const ext4_context* ctx, uint32_t dir_inode_number,
                  dir_entry_callback callback, void* userdata);

// Procura uma entrada com nome 'name' dentro do diretório 'dir_inode_number'.
// Se encontrar, escreve o número do inode em *out_inode_number.
// Retorna 0 se encontrou, -1 se não encontrou.
int ext4_lookup(const ext4_context* ctx, uint32_t dir_inode_number,
                const char* name, uint32_t* out_inode_number);

// Resolve um caminho absoluto ou relativo a partir do diretório corrente do ctx.
// Escreve o inode resultante em *out_inode_number.
// Retorna 0 em sucesso, negativo em erro.
int ext4_resolve_path(const ext4_context* ctx, const char* path,
                      uint32_t* out_inode_number);

// Renomeia a entrada 'old_name' no diretório 'dir_inode_number' para 'new_name'.
// Escreve a alteração diretamente na imagem.
// Retorna 0 em sucesso, negativo em erro.
int ext4_rename_entry(ext4_context* ctx, uint32_t dir_inode_number,
                      const char* old_name, const char* new_name);

// Adiciona uma nova dir entry no diretório 'dir_inode_number'.
// Retorna 0 em sucesso, negativo se não houver espaço ou erro.
int ext4_add_dir_entry(ext4_context* ctx, uint32_t dir_inode_number,
                       uint32_t new_inode_number, const char* name, uint8_t file_type);

// Remove a dir entry com 'name' do diretório 'dir_inode_number'
// marcando seu inode como 0 (entrada deletada).
// Retorna 0 em sucesso, negativo se não encontrou ou erro.
int ext4_remove_dir_entry(ext4_context* ctx, uint32_t dir_inode_number, const char* name);

// Helper: dado o inode de um diretório, retorna o offset (em bytes na imagem) e
// o tamanho em bytes do primeiro bloco de dados deste diretório via extents.
int get_dir_first_block(const ext4_context* ctx, const ext4_inode* dir_inode, off_t* out_offset, uint32_t* out_size);