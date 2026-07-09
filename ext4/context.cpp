#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>

#include "context.h"
#include "../parsers/superblock_parser.h"
#include "../parsers/block_group_descriptor_parser.h"

int ext4_context_init(ext4_context* ctx, const char* image_path) {
    if (ctx == nullptr || image_path == nullptr) return -1;

    memset(ctx, 0, sizeof(*ctx));

    ctx->fd = open(image_path, O_RDWR);
    if (ctx->fd < 0) {
        std::cerr << "Não foi possível abrir a imagem: " << image_path << std::endl;
        return -2;
    }

    /// Leitura do super bloco
    char sb_buf[1024];
    // Pula os primeiros 1024 bytes (boot sector) e lê o superbloco
    if (lseek(ctx->fd, 1024, SEEK_SET) < 0 ||
        read(ctx->fd, sb_buf, sizeof(sb_buf)) != sizeof(sb_buf)) {
        std::cerr << "Erro ao ler o superbloco." << std::endl;
        close(ctx->fd);
        return -3;
    }

    if (parse_superblock(sb_buf, &ctx->sb) != 0) {
        std::cerr << "Erro ao analisar o superbloco." << std::endl;
        close(ctx->fd);
        return -4;
    }

    // Checa se o magic number é o esperado para EXT4 (baseado em: https://blogs.oracle.com/linux/understanding-ext4-disk-layout-part-1)
    if (ctx->sb.s_magic != 0xEF53) {
        std::cerr << "A imagem não é um sistema de arquivos EXT4 válido." << std::endl;
        close(ctx->fd);
        return -5;
    }

    // Checa se as features incompatíveis incluem extents (0x40) - vi que essa flag é só no ext4 em comparação com o ext3, então é uma boa forma de checar se é ext4 mesmo
    if ((ctx->sb.s_feature_incompat & 0x40) != 0x40) {
        std::cerr << "O sistema de arquivos EXT4 desse projeto depende do recurso de extents" << std::endl;
        close(ctx->fd);
        return -6;
    }

    ctx->block_size = 1024 << ctx->sb.s_log_block_size; // Transforma o log do tamanho do bloco em bytes
    ctx->descriptor_size = static_cast<uint16_t>(ctx->sb.s_desc_size); // Tamanho do descritor de grupo de blocos

    ctx->num_groups = (ctx->sb.s_blocks_count_lo + ctx->sb.s_blocks_per_group - 1) / ctx->sb.s_blocks_per_group; // Calcula o número de grupos de blocos (é igual ao número de descritores de grupo de blocos)

    // Leitura dos descritores de grupo de blocos
    const uint32_t bgdt_offset = (ctx->sb.s_log_block_size == 0)
                              ? 2048 // Se for 1KiB, o BGDT começa no bloco 2 (offset 2048)
                              : ctx->block_size; 

    ctx->bgds = new ext4_block_group_descriptor[ctx->num_groups];
    for (int i = 0; i < ctx->num_groups; i++) {
        char* buf = new char[ctx->descriptor_size];

        // Pula para o offset do descritor de grupo de blocos e lê o descritor (i é o índice do grupo de blocos sendo até num_groups-1)
        if (lseek(ctx->fd, bgdt_offset + i * ctx->descriptor_size, SEEK_SET) < 0 ||
            read(ctx->fd, buf, ctx->descriptor_size) != ctx->descriptor_size) {
            std::cerr << "Erro ao ler o descritor de grupo de blocos " << i << std::endl;
            delete[] buf;
            ext4_context_destroy(ctx);
            return -8;
        }

        // Coloca isso no context
        parse_block_group_descriptor(buf, &ctx->bgds[i]);
        delete[] buf;
    }

    // Leitura dos bitmaps de blocos
    const uint32_t bbmap_size = ctx->sb.s_blocks_per_group / 8; // Dividido por 8 porque cada bit representa um bloco

    ctx->block_bitmaps = new char*[ctx->num_groups];
    for (int i = 0; i < ctx->num_groups; i++) {
        ctx->block_bitmaps[i] = new char[bbmap_size];

        const uint32_t off = (ctx->bgds[i].bg_block_bitmap_lo) * ctx->block_size;
        // Pula para o offset do bitmap de blocos e lê o bitmap
        if (lseek(ctx->fd, off, SEEK_SET) < 0 ||
            read(ctx->fd, ctx->block_bitmaps[i], bbmap_size) != bbmap_size) {
            std::cerr << "Erro ao ler o bitmap de blocos do grupo " << i << std::endl;
            ext4_context_destroy(ctx);
            return -9;
        }
    }

    // Leitura dos bitmaps de inodes
    const uint32_t ibmap_size = ctx->sb.s_inodes_per_group / 8; // Dividido por 8 porque cada bit representa um inode

    ctx->inode_bitmaps = new char*[ctx->num_groups];
    for (int i = 0; i < ctx->num_groups; i++) {
        ctx->inode_bitmaps[i] = new char[ibmap_size];

        const uint32_t off = (ctx->bgds[i].bg_inode_bitmap_lo) * ctx->block_size;
        // Pula para o offset do bitmap de inodes e lê o bitmap
        if (lseek(ctx->fd, off, SEEK_SET) < 0 ||
            read(ctx->fd, ctx->inode_bitmaps[i], ibmap_size) != ibmap_size) {
            std::cerr << "Erro ao ler o bitmap de inodes do grupo " << i << std::endl;
            ext4_context_destroy(ctx);
            return -10;
        }
    }

    // Começa na raiz
    ctx->current_inode_number = EXT4_ROOT_INO;
    strncpy(ctx->current_path, "/", sizeof(ctx->current_path) - 1);

    return 0;
}

int ext4_context_save(ext4_context* ctx) {
    if (ctx == nullptr) return -1;

    // Salva o superbloco
    if (lseek(ctx->fd, 1024, SEEK_SET) < 0) {
        std::cerr << "Erro ao salvar o superbloco." << std::endl;
        return -2;
    }

    // Escreve o superbloco de volta na imagem do arquivo
    char sb_buf[1024];
    if (write_superblock(&ctx->sb, sb_buf) != 0) {
        std::cerr << "Erro ao escrever o superbloco no buffer." << std::endl;
        return -3;
    }

    if (write(ctx->fd, sb_buf, sizeof(sb_buf)) != sizeof(sb_buf)) {
        std::cerr << "Erro ao salvar o superbloco." << std::endl;
        return -4;
    }

    // Salva os bitmaps de blocos
    for (int i = 0; i < ctx->num_groups; i++) {
        const uint32_t off = (ctx->bgds[i].bg_block_bitmap_lo) * ctx->block_size;
        if (lseek(ctx->fd, off, SEEK_SET) < 0 ||
            write(ctx->fd, ctx->block_bitmaps[i], ctx->sb.s_blocks_per_group / 8) != static_cast<ssize_t>(ctx->sb.s_blocks_per_group / 8)) {
            std::cerr << "Erro ao salvar o bitmap de blocos do grupo " << i << std::endl;
            return -5;
        }
    }

    // Salva os bitmaps de inodes
    for (int i = 0; i < ctx->num_groups; i++) {
        const uint32_t off = (ctx->bgds[i].bg_inode_bitmap_lo) * ctx->block_size;
        if (lseek(ctx->fd, off, SEEK_SET) < 0 ||
            write(ctx->fd, ctx->inode_bitmaps[i], ctx->sb.s_inodes_per_group / 8) != static_cast<ssize_t>(ctx->sb.s_inodes_per_group / 8)) {
            std::cerr << "Erro ao salvar o bitmap de inodes do grupo " << i << std::endl;
            return -6;
        }
    }

    return 0;
}

void ext4_context_destroy(ext4_context* ctx) {
    if (ctx == nullptr) return;

    if (ctx->block_bitmaps != nullptr) {
        for (int i = 0; i < ctx->num_groups; i++) {
            delete[] ctx->block_bitmaps[i];
        }
        delete[] ctx->block_bitmaps;
        ctx->block_bitmaps = nullptr;
    }

    if (ctx->inode_bitmaps != nullptr) {
        for (int i = 0; i < ctx->num_groups; i++) {
            delete[] ctx->inode_bitmaps[i];
        }
        delete[] ctx->inode_bitmaps;
        ctx->inode_bitmaps = nullptr;
    }

    delete[] ctx->bgds;
    ctx->bgds = nullptr;

    if (ctx->fd >= 0) {
        close(ctx->fd);
        ctx->fd = -1;
    }
}
