#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <iostream>

#include "inode.h"
#include "bitmap.h"

#include "../parsers/inode_parser.h"

int ext4_get_inode(const ext4_context* ctx, uint32_t inode_number, ext4_inode* out_inode) {
    if (ctx == nullptr || out_inode == nullptr) return -1;
    if (inode_number == 0 || inode_number > ctx->sb.s_inodes_count) return -2;
    if (ctx->sb.s_inode_size < sizeof(ext4_inode)) return -5;

    const uint32_t index     = inode_number - 1;
    const uint32_t group     = index / ctx->sb.s_inodes_per_group; // Calcula o grupo de blocos do inode
    const uint32_t idx_group = index % ctx->sb.s_inodes_per_group; // Calcula o índice do inode dentro do grupo

    if (static_cast<int>(group) >= ctx->num_groups) return -3;

    const uint32_t inode_size = ctx->sb.s_inode_size; // Tamanho do inode (em bytes) definido no superbloco
    const uint64_t table_off  = static_cast<uint64_t>(ctx->bgds[group].bg_inode_table_lo) * ctx->block_size; // Offset do início da tabela de inodes do grupo (em bytes)
    const uint64_t inode_off  = table_off + static_cast<uint64_t>(idx_group) * inode_size;

    char* buf = new char[inode_size];
    // Lê o inode da imagem do arquivo
    if (lseek(ctx->fd, static_cast<off_t>(inode_off), SEEK_SET) < 0 ||
        read(ctx->fd, buf, inode_size) != static_cast<ssize_t>(inode_size)) {
        delete[] buf;
        return -4;
    }

    // Faz o parse do inode
    const int ret = parse_inode(buf, out_inode);
    delete[] buf;
    return ret;
}

int ext4_read_inode_data(const ext4_context* ctx, const ext4_inode* inode,
                         char** out_data, uint32_t* out_size) {
    if (ctx == nullptr || inode == nullptr || out_data == nullptr || out_size == nullptr) return -1;

    const ext4_extent_header* hdr =
        reinterpret_cast<const ext4_extent_header*>(inode->i_block);

    // Verifica se o inode possui extents válidos (eh_magic) e se é um extent simples (eh_depth == 0)
    if (hdr->eh_magic != EXT4_EXTENT_MAGIC) return -2;
    if (hdr->eh_depth != 0) return -3; // arvore de extents nao suportada

    const ext4_extent* extents = reinterpret_cast<const ext4_extent*>(
        inode->i_block + sizeof(ext4_extent_header));

    // Calcula total de bytes para já prealocar o buffer de leitura. Se o inode tiver mais de 32768 blocos em um extent, o tamanho real é len - 32768.
    uint32_t total_bytes = 0;
    for (int i = 0; i < hdr->eh_entries; i++) {
        // Calcula o tamanho em blocos do extent. Se for > 32768, é um valor especial (bit 15 setado) e o tamanho real é len - 32768.
        uint16_t len = extents[i].ee_len;
        if (len > 32768) len = static_cast<uint16_t>(len - 32768); // uninit extent
        total_bytes += static_cast<uint32_t>(len) * ctx->block_size;
    }

    if (total_bytes == 0) {
        *out_data = nullptr;
        *out_size = 0;
        return 0;
    }

    char* buf = new char[total_bytes];
    uint32_t written = 0;

    // Lê os blocos de dados do inode via extents
    for (int i = 0; i < hdr->eh_entries; i++) {
        uint16_t len = extents[i].ee_len;
        if (len > 32768) len = static_cast<uint16_t>(len - 32768);
        const uint32_t bytes = static_cast<uint32_t>(len) * ctx->block_size;
        const off_t off = static_cast<off_t>(extents[i].ee_start_lo) * ctx->block_size;

        if (lseek(ctx->fd, off, SEEK_SET) < 0 ||
            read(ctx->fd, buf + written, bytes) != static_cast<ssize_t>(bytes)) {
            delete[] buf;
            return -4;
        }
        written += bytes;
    }

    *out_data = buf;
    *out_size = written;
    return 0;
}

int ext4_write_inode(const ext4_context* ctx, uint32_t inode_number, const ext4_inode* inode) {
    if (ctx == nullptr || inode == nullptr) return -1;
    if (inode_number == 0 || inode_number > ctx->sb.s_inodes_count) return -2;
    if (ctx->sb.s_inode_size < sizeof(ext4_inode)) return -5;

    const uint32_t index     = inode_number - 1;
    const uint32_t group     = index / ctx->sb.s_inodes_per_group; // Calcula o grupo de blocos do inode
    const uint32_t idx_group = index % ctx->sb.s_inodes_per_group; // Calcula o índice do inode dentro do grupo

    if (static_cast<int>(group) >= ctx->num_groups) return -3; // Checa se o grupo é válido
    const uint32_t inode_size = ctx->sb.s_inode_size; // Tamanho do inode (em bytes) definido no superbloco
    const uint64_t table_off = static_cast<uint64_t>(ctx->bgds[group].bg_inode_table_lo) * ctx->block_size; // Offset do início da tabela de inodes do grupo (em bytes)
    const uint64_t inode_off = table_off + static_cast<uint64_t>(idx_group) * inode_size;

    // Escreve apenas os campos que parse_inode conhece (sizeof ext4_inode bytes)
    if (lseek(ctx->fd, static_cast<off_t>(inode_off), SEEK_SET) < 0 ||
        write(ctx->fd, inode, sizeof(ext4_inode)) != sizeof(ext4_inode)) {
        return -4;
    }
    return 0;
}

uint32_t ext4_alloc_and_zero_block(ext4_context* ctx) {
    uint32_t block_number = 0;
    if (ext4_alloc_block(ctx, &block_number) != 0) return 0;

    // Cria um bloco de dados zerado na imagem do arquivo
    char* zeros = new char[ctx->block_size]();
    const off_t off = static_cast<off_t>(block_number) * ctx->block_size;
    lseek(ctx->fd, off, SEEK_SET);
    write(ctx->fd, zeros, ctx->block_size);
    delete[] zeros;
    
    // Não atualiza o contador do superbloco uma vez que ext4_alloc_block já faz isso
    return block_number;
}
