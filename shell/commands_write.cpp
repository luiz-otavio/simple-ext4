#include <iostream>
#include <cstring>
#include <ctime>
#include <unistd.h>

#include "commands_write.h"

#include "../ext4/inode.h"
#include "../ext4/dir.h"
#include "../ext4/bitmap.h"

// Comando RENAME que renomeia um arquivo ou diretório de 'name' para 'newname'
void cmd_rename(ext4_context* ctx, const char* name, const char* newname) {
    uint32_t dummy = 0;
    // Procura por um inode com o nome 'name' no diretório corrente
    if (ext4_lookup(ctx, ctx->current_inode_number, name, &dummy) != 0) {
        std::cerr << "rename: não encontrado: " << name << std::endl;
        return;
    }

    // Verifica se já existe um arquivo ou diretório com o nome 'newname' no diretório corrente
    if (ext4_rename_entry(ctx, ctx->current_inode_number, name, newname) != 0) {
        std::cerr << "rename: falhou (nome já existe ou não há espaço para o novo nome na entrada de diretório)." << std::endl;
        return;
    }
    std::cout << "Renomeado '" << name << "' para '" << newname << "'." << std::endl;
}

// Comando TOUCH que cria um arquivo vazio com o nome especificado
void cmd_touch(ext4_context* ctx, const char* name) {
    // Verifica se já existe
    uint32_t existing = 0;
    // Procurar por um inode com o nome 'name' no diretório corrente
    if (ext4_lookup(ctx, ctx->current_inode_number, name, &existing) == 0) {
        std::cout << "touch: '" << name << "' já existe." << std::endl;
        return;
    }

    uint32_t new_inode_number = 0;
    // Aloca um novo inode no bitmap de inodes do contexto
    if (ext4_alloc_inode(ctx, &new_inode_number) != 0) {
        std::cerr << "touch: sem inodes livres." << std::endl;
        return;
    }

    // Cria a estrutura inode para ser colocada no novo inode alocado, inicializando os campos necessários para um arquivo regular vazio
    ext4_inode inode;
    memset(&inode, 0, sizeof(inode));
    inode.i_mode       = 0x81A4; // regular file, 0644
    inode.i_links_count = 1;
    inode.i_flags      = EXT4_EXTENTS_FL;
    const uint32_t now = static_cast<uint32_t>(time(nullptr));
    inode.i_atime = inode.i_ctime = inode.i_mtime = now;

    // Inicializa extent header vazio dentro de i_block
    ext4_extent_header hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.eh_magic = EXT4_EXTENT_MAGIC;
    hdr.eh_max   = 4;
    memcpy(inode.i_block, &hdr, sizeof(hdr));

    // Escreve o inode no número de inode alocado
    if (ext4_write_inode(ctx, new_inode_number, &inode) != 0) {
        std::cerr << "touch: falha ao escrever inode." << std::endl;
        return;
    }

    // Adiciona a entrada de diretório para o novo arquivo no diretório corrente
    if (ext4_add_dir_entry(ctx, ctx->current_inode_number, new_inode_number, name, EXT4_FT_REG_FILE) != 0) {
        std::cerr << "touch: sem espaço no bloco de diretório." << std::endl;
        return;
    }

    std::cout << "Criado '" << name << "' (inode " << new_inode_number << ")." << std::endl;
}

// Comando MKDIR que cria um diretório com o nome especificado
void cmd_mkdir(ext4_context* ctx, const char* name) {
    uint32_t existing = 0;
    // Procura por um inode com o nome 'name' no diretório corrente
    if (ext4_lookup(ctx, ctx->current_inode_number, name, &existing) == 0) {
        std::cerr << "mkdir: já existe: " << name << std::endl;
        return;
    }

    uint32_t new_inode_number = 0;
    // Aloca um novo inode para o diretório
    if (ext4_alloc_inode(ctx, &new_inode_number) != 0) {
        std::cerr << "mkdir: sem inodes livres." << std::endl;
        return;
    }

    // Aloca bloco de dados para as entradas "." e ".." 
    const uint32_t data_block = ext4_alloc_and_zero_block(ctx);
    if (data_block == 0) {
        std::cerr << "mkdir: sem blocos livres." << std::endl;
        return;
    }

    // Cria a estrutura inode para o novo diretório, inicializando os campos necessários para um diretório vazio
    ext4_inode inode;
    memset(&inode, 0, sizeof(inode));
    inode.i_mode        = 0x41ED; // directory, 0755
    inode.i_links_count = 2;      // "." e entrada no pai
    inode.i_flags       = EXT4_EXTENTS_FL;
    inode.i_size_lo     = ctx->block_size;
    inode.i_blocks_lo   = ctx->block_size / 512; // Padrão POSIX: número de blocos de 512 bytes
    const uint32_t now = static_cast<uint32_t>(time(nullptr));
    inode.i_atime = inode.i_ctime = inode.i_mtime = now; // Aloca o tempo atual para os campos de tempo do inode

    // Extent apontando para o bloco de dados
    ext4_extent_header hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.eh_magic   = EXT4_EXTENT_MAGIC;
    hdr.eh_entries = 1;
    hdr.eh_max     = 4;
    ext4_extent ext;
    memset(&ext, 0, sizeof(ext));
    ext.ee_block    = 0;
    ext.ee_len      = 1;
    ext.ee_start_lo = data_block;
    memcpy(inode.i_block, &hdr, sizeof(hdr));
    memcpy(inode.i_block + sizeof(hdr), &ext, sizeof(ext));

    // Escreve o inode no número de inode alocado
    if (ext4_write_inode(ctx, new_inode_number, &inode) != 0) {
        std::cerr << "mkdir: falha ao escrever inode." << std::endl;
        return;
    }

    // Adiciona "." e ".." no bloco de dados do novo diretório
    if (ext4_add_dir_entry(ctx, new_inode_number, new_inode_number, ".", EXT4_FT_DIR) != 0 ||
        ext4_add_dir_entry(ctx, new_inode_number, ctx->current_inode_number, "..", EXT4_FT_DIR) != 0) {
        std::cerr << "mkdir: falha ao inicializar entradas '.' e '..'." << std::endl;
        return;
    }

    // Adiciona entrada no diretório pai
    if (ext4_add_dir_entry(ctx, ctx->current_inode_number, new_inode_number, name, EXT4_FT_DIR) != 0) {
        std::cerr << "mkdir: sem espaço no bloco do diretório pai." << std::endl;
        return;
    }

    std::cout << "Criado diretório '" << name << "' (inode " << new_inode_number << ")." << std::endl;
}

// Comando RM que remove um arquivo com o nome especificado
void cmd_rm(ext4_context* ctx, const char* name) {
    uint32_t inode_number = 0;
    if (ext4_lookup(ctx, ctx->current_inode_number, name, &inode_number) != 0) {
        std::cerr << "rm: não encontrado: " << name << std::endl;
        return;
    }

    ext4_inode inode;
    // Obtém o inode do arquivo para verificar se é um diretório ou um arquivo regular
    if (ext4_get_inode(ctx, inode_number, &inode) != 0) { std::cerr << "rm: não é possível ler o inode." << std::endl; return; }

    // Checa se o inode é um diretório, e se for, informa ao usuário para usar rmdir em vez de rm (se a implementação fosse mais especifica, adicionaria o -rf)
    if ((inode.i_mode & 0xF000) == 0x4000) {
        std::cerr << "rm: é um diretório (use rmdir): " << name << std::endl;
        return;
    }

    // Remove a entrada de diretório do arquivo no diretório corrente
    if (ext4_remove_dir_entry(ctx, ctx->current_inode_number, name) != 0) {
        std::cerr << "rm: falha ao remover entrada do diretório." << std::endl;
        return;
    }

    // Decrementa links
    inode.i_links_count--;
    // Se o número de links chegar a 0, marca o inode como deletado e libera no bitmap
    if (inode.i_links_count == 0) {
        inode.i_dtime = static_cast<uint32_t>(time(nullptr));
        ext4_write_inode(ctx, inode_number, &inode);
        ext4_free_inode(ctx, inode_number);
    } else {
        ext4_write_inode(ctx, inode_number, &inode);
    }

    std::cout << "Removido '" << name << "'." << std::endl;
}

// Comando RMDIR que remove um diretório vazio com o nome especificado
void cmd_rmdir(ext4_context* ctx, const char* name) {
    uint32_t inode_number = 0;
    // Procura por um inode com o nome 'name' no diretório corrente
    if (ext4_lookup(ctx, ctx->current_inode_number, name, &inode_number) != 0) {
        std::cerr << "rmdir: não encontrado: " << name << std::endl;
        return;
    }

    ext4_inode inode;
    // Obtém o inode do diretório para verificar se é realmente um diretório e se está vazio
    if (ext4_get_inode(ctx, inode_number, &inode) != 0) { std::cerr << "rmdir: não é possível ler o inode." << std::endl; return; }

    // Verifica se o inode é realmente um diretório com base no i_mode (do bit 15 ao bit 12)
    if ((inode.i_mode & 0xF000) != 0x4000) {
        std::cerr << "rmdir: não é um diretório: " << name << std::endl;
        return;
    }

    // Verifica se está vazio (só "." e "..")
    int entry_count = 0;
    ext4_list_dir(ctx, inode_number, [](const ext4_dir_entry* e, void* ud) -> int {
        if (strcmp(e->name, ".") != 0 && strcmp(e->name, "..") != 0)
            (*reinterpret_cast<int*>(ud))++;
        return 0;
    }, &entry_count);

    // Se o diretório não estiver vazio, informa ao usuário que não é possível remover o diretório (implementação simples)
    if (entry_count > 0) {
        std::cerr << "rmdir: diretório não está vazio: " << name << std::endl;
        return;
    }

    // Remove a entrada de diretório do diretório no diretório corrente
    if (ext4_remove_dir_entry(ctx, ctx->current_inode_number, name) != 0) {
        std::cerr << "rmdir: falha ao remover entrada do diretório." << std::endl;
        return;
    }

    inode.i_dtime = static_cast<uint32_t>(time(nullptr));
    inode.i_links_count = 0;
    ext4_write_inode(ctx, inode_number, &inode);
    ext4_free_inode(ctx, inode_number);

    off_t block_off = 0;
    uint32_t block_size = 0;
    // Obtém o primeiro bloco de dados do diretório via extents
    if (get_dir_first_block(ctx, &inode, &block_off, &block_size) != 0) return;

    // Libera o bloco de dados do diretório
    ext4_free_block(ctx, block_off / ctx->block_size);

    std::cout << "Removido diretório '" << name << "'." << std::endl;
}
