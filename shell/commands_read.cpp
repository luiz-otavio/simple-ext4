#include <iostream>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <unistd.h>

#include "commands_read.h"
#include "../ext4/inode.h"
#include "../ext4/dir.h"
#include "../ext4/bitmap.h"

// Comando info que mostra informações do superbloco e do contexto
void cmd_info(ext4_context* ctx) {
    const ext4_superblock& sb = ctx->sb;
    std::cout << "Volume name:      " << sb.s_volume_name << std::endl;
    std::cout << "Block size:       " << ctx->block_size << " bytes" << std::endl;
    std::cout << "Total blocks:     " << sb.s_blocks_count_lo << std::endl;
    std::cout << "Free blocks:      " << sb.s_free_blocks_count_lo << std::endl;
    std::cout << "Total inodes:     " << sb.s_inodes_count << std::endl;
    std::cout << "Free inodes:      " << sb.s_free_inodes_count << std::endl;
    std::cout << "Inodes per group: " << sb.s_inodes_per_group << std::endl;
    std::cout << "Blocks per group: " << sb.s_blocks_per_group << std::endl;
    std::cout << "Block groups:     " << ctx->num_groups << std::endl;
}

// Auxiliar pro comando ls, que imprime o nome de cada entrada de diretório
int print_entry(const ext4_dir_entry* e, void*) {
    std::cout << e->name;
    // Checa se é um diretório para adicionar uma barra no final
    if (e->file_type == EXT4_FT_DIR) std::cout << "/";
    std::cout << std::endl;
    return 0;
}

// Comando LS que usa o list_dir para listar os arquivos e diretórios do diretório corrente
void cmd_ls(ext4_context* ctx) {
    ext4_list_dir(ctx, ctx->current_inode_number, print_entry, nullptr);
}

// Comando PWD que mostra o caminho absoluto do diretório corrente
void cmd_pwd(ext4_context* ctx) {
    std::cout << ctx->current_path << std::endl;
}

// Comando CD que muda o diretório corrente para o caminho especificado
void cmd_cd(ext4_context* ctx, const char* path) {
    if (path == nullptr || path[0] == '\0' || strcmp(path, ".") == 0) {
        return;
    }

    uint32_t target_inode = 0;
    // Checa se existe o caminho especificado e obtém o inode correspondente
    if (ext4_resolve_path(ctx, path, &target_inode) != 0) {
        std::cerr << "cd: diretório não encontrado: " << path << std::endl;
        return;
    }

    ext4_inode inode;
    // Pega o inode do diretório alvo para checar se é realmente um diretório
    if (ext4_get_inode(ctx, target_inode, &inode) != 0) {
        std::cerr << "cd: não foi possível ler o inode." << std::endl;
        return;
    }

    // Checa se o inode é um diretório (i_mode & 0xF000 == 0x4000)
    if ((inode.i_mode & 0xF000) != 0x4000) {
        std::cerr << "cd: não é um diretório: " << path << std::endl;
        return;
    }

    // Atualiza o inode do diretório corrente no contexto
    ctx->current_inode_number = target_inode;

    // Atualiza current_path.
    if (path[0] == '/') {
        strncpy(ctx->current_path, path, sizeof(ctx->current_path) - 1);
        ctx->current_path[sizeof(ctx->current_path) - 1] = '\0';

        const size_t len = strlen(ctx->current_path);
        if (len > 0 && ctx->current_path[len - 1] != '/') {
            strncat(ctx->current_path, "/", sizeof(ctx->current_path) - len - 1);
        }
    } else if (strcmp(path, "..") == 0) {
        size_t len = strlen(ctx->current_path);

        if (len > 1 && ctx->current_path[len - 1] == '/') {
            ctx->current_path[len - 1] = '\0';
        }

        char* last = strrchr(ctx->current_path, '/');
        if (last != nullptr && last != ctx->current_path) {
            *(last + 1) = '\0';
        } else {
            strcpy(ctx->current_path, "/");
        }
    } else {
        const size_t len = strlen(ctx->current_path);
        if (len > 0 && ctx->current_path[len - 1] != '/') {
            strncat(ctx->current_path, "/", sizeof(ctx->current_path) - len - 1);
        }
        strncat(ctx->current_path, path, sizeof(ctx->current_path) - strlen(ctx->current_path) - 1);

        const size_t new_len = strlen(ctx->current_path);
        if (new_len > 0 && ctx->current_path[new_len - 1] != '/') {
            strncat(ctx->current_path, "/", sizeof(ctx->current_path) - new_len - 1);
        }
    }
}

// Comando CAT que mostra o conteúdo de um arquivo especificado pelo caminho
void cmd_cat(ext4_context* ctx, const char* path) {
    uint32_t inode_number = 0;
    // Resolve o caminho para obter o número do inode correspondente
    if (ext4_resolve_path(ctx, path, &inode_number) != 0) {
        std::cerr << "cat: arquivo não encontrado: " << path << std::endl;
        return;
    }

    ext4_inode inode;
    // Obtém o inode do arquivo para ler seus dados
    if (ext4_get_inode(ctx, inode_number, &inode) != 0) { std::cerr << "cat: não foi possível ler o inode." << std::endl; return; }

    char* data = nullptr;
    uint32_t size = 0;
    // Lê os dados do inode para obter o conteúdo do arquivo
    if (ext4_read_inode_data(ctx, &inode, &data, &size) != 0) {
        std::cerr << "cat: não foi possível ler os dados do arquivo." << std::endl;
        return;
    }

    // Calcula o tamanho real do arquivo (considerando i_size_high e i_size_lo) e imprime o conteúdo
    const uint64_t file_size = static_cast<uint64_t>(inode.i_size_lo) |
                               (static_cast<uint64_t>(inode.i_size_high) << 32);
                            
    // Aqui é preciso garantir que não vamos imprimir mais bytes do que o tamanho real do arquivo, mesmo que o buffer lido seja maior
    // já que o inode tenha sido alocado com blocos completos, mas o arquivo pode ser menor.
    const uint32_t print_size = (file_size < size) ? static_cast<uint32_t>(file_size) : size;
    std::cout.write(data, print_size);
    std::cout << std::endl;
    delete[] data;
}

// Comando ATTR que mostra os atributos do arquivo ou diretório especificado pelo caminho
void cmd_attr(ext4_context* ctx, const char* path) {
    uint32_t inode_number = 0;
    // Resolve o caminho para obter o número do inode correspondente
    if (ext4_resolve_path(ctx, path, &inode_number) != 0) {
        std::cerr << "attr: arquivo não encontrado: " << path << std::endl;
        return;
    }

    ext4_inode inode;
    // Obtém o inode do arquivo ou diretório para ler seus atributos
    if (ext4_get_inode(ctx, inode_number, &inode) != 0) { std::cerr << "attr: não foi possível ler o inode." << std::endl; return; }

    // Calcula o tamanho real do arquivo (considerando i_size_high e i_size_lo)
    const uint64_t size = static_cast<uint64_t>(inode.i_size_lo) |
                          (static_cast<uint64_t>(inode.i_size_high) << 32);

    // Aqui eu preciso determinar se o inode é um diretório ou um arquivo regular para exibir o tipo correto com base no i_mode.
    // O tipo é determinado pelos bits mais significativos do i_mode, onde 0x4000 indica um diretório e 0x8000 indica um arquivo regular.
    const char* type = ((inode.i_mode & 0xF000) == 0x4000) ? "directory" : "file";

    std::cout << "Inode:       " << inode_number << std::endl;
    std::cout << "Type:        " << type << std::endl;
    std::cout << "Size:        " << size << " bytes" << std::endl;
    std::cout << "Hard links:  " << inode.i_links_count << std::endl;
    std::cout << "Mode:        0" << std::oct << (inode.i_mode & 0xFFF) << std::dec << std::endl;
    std::cout << "UID/GID:     " << inode.i_uid << "/" << inode.i_gid << std::endl;

    time_t t = static_cast<time_t>(inode.i_mtime);
    std::cout << "Modified:    " << ctime(&t);
}

// Comando TESTI que testa se um inode está em uso ou não, baseado no bitmap de inodes
void cmd_testi(ext4_context* ctx, uint32_t inode_number) {
    // Checa se o inode está em uso usando a função ext4_is_inode_used
    const int used = ext4_is_inode_used(ctx, inode_number);
    if (used < 0) { std::cerr << "testi: inode inválido." << std::endl; return; }
    std::cout << "Inode " << inode_number << " está " << (used ? "em uso" : "livre") << "." << std::endl;
}

// Comando TESTB que testa se um bloco está em uso ou não, baseado no bitmap de blocos
void cmd_testb(ext4_context* ctx, uint32_t block_number) {
    // Checa se o bloco está em uso usando a função ext4_is_block_used
    const int used = ext4_is_block_used(ctx, block_number);
    if (used < 0) { std::cerr << "testb: bloco inválido." << std::endl; return; }
    std::cout << "Bloco " << block_number << " está " << (used ? "em uso" : "livre") << "." << std::endl;
}

// Comando EXPORT que exporta um arquivo do sistema de arquivos EXT4 para o sistema de arquivos do host
void cmd_export(ext4_context* ctx, const char* source, const char* dest) {
    uint32_t inode_number = 0;
    if (ext4_resolve_path(ctx, source, &inode_number) != 0) {
        std::cerr << "export: source não encontrado: " << source << std::endl;
        return;
    }

    ext4_inode inode;
    if (ext4_get_inode(ctx, inode_number, &inode) != 0) { std::cerr << "export: não é possível ler o inode." << std::endl; return; }

    char* data = nullptr;
    uint32_t size = 0;
    if (ext4_read_inode_data(ctx, &inode, &data, &size) != 0) {
        std::cerr << "export: não é possível ler os dados do arquivo." << std::endl;
        return;
    }

    const uint64_t file_size = static_cast<uint64_t>(inode.i_size_lo) |
                               (static_cast<uint64_t>(inode.i_size_high) << 32);
    const uint32_t write_size = (file_size < size) ? static_cast<uint32_t>(file_size) : size;

    const int out_fd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (out_fd < 0) {
        std::cerr << "export: não é possível criar o arquivo: " << dest << std::endl;
        delete[] data;
        return;
    }

    if (write(out_fd, data, write_size) != static_cast<ssize_t>(write_size)) {
        std::cerr << "export: erro ao escrever no arquivo: " << dest << std::endl;
    }
    close(out_fd);
    delete[] data;
    std::cout << "Exportado " << write_size << " bytes para " << dest << std::endl;
}
