#include <cstring>
#include <unistd.h>

#include "bitmap.h"

// Funções auxiliares para manipulação de bitmaps
int bitmap_get_bit(const char* bitmap, uint32_t index) {
    return (bitmap[index / 8] >> (index % 8)) & 1;
}

void bitmap_set_bit(char* bitmap, uint32_t index) {
    bitmap[index / 8] |= static_cast<char>(1 << (index % 8));
}

void bitmap_clear_bit(char* bitmap, uint32_t index) {
    bitmap[index / 8] &= static_cast<char>(~(1 << (index % 8)));
}

int ext4_is_inode_used(const ext4_context* ctx, uint32_t inode_number) {
    if (ctx == nullptr || inode_number == 0 || inode_number > ctx->sb.s_inodes_count) return -1;

    const uint32_t index  = inode_number - 1;
    const uint32_t group  = index / ctx->sb.s_inodes_per_group; // Calcula o grupo de blocos correto para o inode
    const uint32_t offset = index % ctx->sb.s_inodes_per_group; // Pega o offset do inode dentro do grupo de blocos 

    if (static_cast<int>(group) >= ctx->num_groups) return -1; // Se o calculo deu um grupo maior que o número de grupos, retorna erro
    return bitmap_get_bit(ctx->inode_bitmaps[group], offset); 
}

int ext4_is_block_used(const ext4_context* ctx, uint32_t block_number) {
    if (ctx == nullptr || block_number == 0) return -1;

    const uint32_t group  = block_number / ctx->sb.s_blocks_per_group; // Calcula o grupo de blocos correto para o bloco
    const uint32_t offset = block_number % ctx->sb.s_blocks_per_group; // Pega o offset do bloco dentro do grupo de blocos

    if (static_cast<int>(group) >= ctx->num_groups) return -1; // Se o calculo deu um grupo maior que o número de grupos, retorna erro
    return bitmap_get_bit(ctx->block_bitmaps[group], offset);
}

int ext4_alloc_inode(ext4_context* ctx, uint32_t* out_inode_number) {
    if (ctx == nullptr || out_inode_number == nullptr) return -1;

    const uint32_t inodes_in_group = ctx->sb.s_inodes_per_group;
    for (int g = 0; g < ctx->num_groups; g++) {
        for (uint32_t i = 0; i < inodes_in_group; i++) {
            // Procura por um inode livre no bitmap de inodes do grupo
            if (!bitmap_get_bit(ctx->inode_bitmaps[g], i)) {
                bitmap_set_bit(ctx->inode_bitmaps[g], i);

                // Persiste bitmap na imagem
                const uint32_t off = (ctx->bgds[g].bg_inode_bitmap_lo) * ctx->block_size;
                lseek(ctx->fd, off + i / 8, SEEK_SET); // Posiciona o cursor no arquivo da imagem para o byte correto do bitmap
                write(ctx->fd, &ctx->inode_bitmaps[g][i / 8], 1); // Escreve o byte do bitmap na imagem

                // Atualiza o numero de inodes livres
                ctx->sb.s_free_inodes_count--;
                ctx->sb.s_inodes_count++; // Incrementa o contador de inodes alocados

                *out_inode_number = static_cast<uint32_t>(g) * inodes_in_group + i + 1; // Retorna o inode number (1-based) aonde foi alocado
                return 0;
            }
        }
    }
    return -1;
}

int ext4_alloc_block(ext4_context* ctx, uint32_t* out_block_number) {
    if (ctx == nullptr || out_block_number == nullptr) return -1;

    const uint32_t blocks_in_group = ctx->sb.s_blocks_per_group;
    for (int g = 0; g < ctx->num_groups; g++) {
        for (uint32_t i = 0; i < blocks_in_group; i++) {
            if (!bitmap_get_bit(ctx->block_bitmaps[g], i)) {
                bitmap_set_bit(ctx->block_bitmaps[g], i);

                // Persiste bitmap na imagem
                const uint32_t off = (ctx->bgds[g].bg_block_bitmap_lo) * ctx->block_size;
                lseek(ctx->fd, off + i / 8, SEEK_SET); // Posiciona o cursor no arquivo da imagem para o byte correto do bitmap
                write(ctx->fd, &ctx->block_bitmaps[g][i / 8], 1); // Escreve o byte do bitmap na imagem

                // Atualiza o numero de blocos livres
                ctx->sb.s_free_blocks_count_lo--;
                ctx->sb.s_blocks_count_lo++; // Incrementa o contador de blocos alocados no superbloco

                *out_block_number = static_cast<uint32_t>(g) * blocks_in_group + i; // Retorna o block number (0-based) aonde foi alocado
                return 0;
            }
        }
    }
    return -1;
}

int ext4_free_inode(ext4_context* ctx, uint32_t inode_number) {
    if (ctx == nullptr || inode_number == 0 || inode_number > ctx->sb.s_inodes_count) return -1;

    const uint32_t index  = inode_number - 1;
    const uint32_t group  = index / ctx->sb.s_inodes_per_group; // Calcula o grupo de blocos correto para o inode
    const uint32_t offset = index % ctx->sb.s_inodes_per_group; // Pega o offset do inode dentro do grupo de blocos

    if (static_cast<int>(group) >= ctx->num_groups) return -1;

    bitmap_clear_bit(ctx->inode_bitmaps[group], offset);

    const uint32_t bmap_off = ctx->bgds[group].bg_inode_bitmap_lo * ctx->block_size; // Calcula o offset do bitmap de inodes no grupo
    lseek(ctx->fd, bmap_off + offset / 8, SEEK_SET);
    write(ctx->fd, &ctx->inode_bitmaps[group][offset / 8], 1);

    // Atualiza o numero de inodes livres
    ctx->sb.s_free_inodes_count++;
    ctx->sb.s_inodes_count--;

    return 0;
}

int ext4_free_block(ext4_context* ctx, uint32_t block_number) {
    if (ctx == nullptr) return -1;

    const uint32_t group  = block_number / ctx->sb.s_blocks_per_group; // Calcula o grupo de blocos correto para o bloco
    const uint32_t offset = block_number % ctx->sb.s_blocks_per_group; // Pega o offset do bloco dentro do grupo de blocos

    if (static_cast<int>(group) >= ctx->num_groups) return -1;

    bitmap_clear_bit(ctx->block_bitmaps[group], offset);

    const uint32_t bmap_off = ctx->bgds[group].bg_block_bitmap_lo * ctx->block_size; // Calcula o offset do bitmap de blocos no grupo
    lseek(ctx->fd, bmap_off + offset / 8, SEEK_SET);
    write(ctx->fd, &ctx->block_bitmaps[group][offset / 8], 1);

    // Atualiza o numero de blocos livres
    ctx->sb.s_free_blocks_count_lo++;
    ctx->sb.s_blocks_count_lo--;

    return 0;
}
