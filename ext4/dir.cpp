#include <cstring>
#include <unistd.h>

#include "dir.h"
#include "inode.h"

const uint32_t ZERO = 0;

int ext4_list_dir(const ext4_context* ctx, uint32_t dir_inode_number,
                  dir_entry_callback callback, void* userdata) {
    if (ctx == nullptr || callback == nullptr) return -1;

    ext4_inode dir_inode;
    if (ext4_get_inode(ctx, dir_inode_number, &dir_inode) != 0) return -2;

    char* data = nullptr;
    uint32_t data_size = 0;
    if (ext4_read_inode_data(ctx, &dir_inode, &data, &data_size) != 0) return -3;

    uint32_t offset = 0;
    while (offset + 8 <= data_size) {
        const char* ptr = data + offset;

        ext4_dir_entry entry;
        memcpy(&entry.inode,     ptr + 0, sizeof(uint32_t));
        memcpy(&entry.rec_len,   ptr + 4, sizeof(uint16_t));
        memcpy(&entry.name_len,  ptr + 6, sizeof(uint8_t));
        memcpy(&entry.file_type, ptr + 7, sizeof(uint8_t));

        if (entry.rec_len < 8 || offset + entry.rec_len > data_size) break;

        if (entry.inode != 0 && entry.name_len > 0) {
            uint8_t copy_len = entry.name_len;
            if (copy_len > sizeof(entry.name) - 1) copy_len = sizeof(entry.name) - 1;
            memcpy(entry.name, ptr + 8, copy_len);
            entry.name[copy_len] = '\0';
            entry.name_len = copy_len;

            if (callback(&entry, userdata) != 0) break;
        }

        offset += entry.rec_len;
    }

    delete[] data;
    return 0;
}

// Callback interno para ext4_lookup
struct lookup_state {
    const char* target;
    uint32_t    found_inode;
    int         found;
};

int _lookup_callback(const ext4_dir_entry* entry, void* userdata) {
    lookup_state* s = reinterpret_cast<lookup_state*>(userdata);
    if (strcmp(entry->name, s->target) == 0) {
        s->found_inode = entry->inode;
        s->found = 1;
        return 1; // para a iteração
    }
    return 0;
}

int ext4_lookup(const ext4_context* ctx, uint32_t dir_inode_number,
                const char* name, uint32_t* out_inode_number) {
    if (name == nullptr || out_inode_number == nullptr) return -1;

    lookup_state state;
    state.target      = name;
    state.found_inode = 0;
    state.found       = 0;

    // Lista os dir entries do diretório e procura pelo nome
    ext4_list_dir(ctx, dir_inode_number, _lookup_callback, &state);

    if (!state.found || state.found_inode == 0) return -1;
    *out_inode_number = state.found_inode;
    return 0;
}

int ext4_resolve_path(const ext4_context* ctx, const char* path,
                      uint32_t* out_inode_number) {
    if (ctx == nullptr || path == nullptr || out_inode_number == nullptr) return -1;

    uint32_t current = (path[0] == '/') ? EXT4_ROOT_INO : ctx->current_inode_number;

    if (path[0] == '/' && path[1] == '\0') {
        *out_inode_number = EXT4_ROOT_INO;
        return 0;
    }

    // Copia para tokenizar sem modificar o original
    char buf[4096];
    strncpy(buf, path, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char* token = strtok(buf, "/");
    while (token != nullptr) {
        // Checa se o token é "." ou ".." ou um nome de arquivo/diretório
        if (strcmp(token, ".") == 0) {
            // Fica no mesmo
        } else if (strcmp(token, "..") == 0) {
            // Sobe um nível: usa a entrada ".." do diretório corrente
            uint32_t parent = 0;
            if (ext4_lookup(ctx, current, "..", &parent) == 0) {
                current = parent;
            }
        } else {
            // Procura o token no diretório corrente
            uint32_t next = 0;
            if (ext4_lookup(ctx, current, token, &next) != 0) return -2;
            current = next;
        }
        token = strtok(nullptr, "/");
    }

    *out_inode_number = current;
    return 0;
}

// Helper: dado o inode de um diretório, retorna o offset (em bytes na imagem) e
// o tamanho em bytes do primeiro bloco de dados deste diretório via extents.
static int get_dir_first_block(const ext4_context* ctx, const ext4_inode* dir_inode,
                                off_t* out_offset, uint32_t* out_size) {
    const ext4_extent_header* hdr =
        reinterpret_cast<const ext4_extent_header*>(dir_inode->i_block);
    if (hdr->eh_magic != EXT4_EXTENT_MAGIC || hdr->eh_depth != 0 || hdr->eh_entries == 0) return -1;

    // Força o cast para os outros 48 bytes do i_block como ext4_extent, que é o que esperamos para um diretório com extents.
    const ext4_extent* ext = (reinterpret_cast<const ext4_extent*>(dir_inode->i_block + sizeof(ext4_extent_header)));

    uint16_t len = ext->ee_len; // Calcula o tamanho em blocos do extent. Se for > 32768, é um valor especial (bit 15 setado) e o tamanho real é len - 32768.
    if (len > 32768) len = len - 32768;

    *out_offset = (ext->ee_start_lo) * ctx->block_size; // Calcula o offset em bytes na imagem do primeiro bloco do diretório
    *out_size   = (len) * ctx->block_size; // Calcula o tamanho em bytes do extent (len blocos * tamanho do bloco)
    return 0;
}

int ext4_rename_entry(ext4_context* ctx, uint32_t dir_inode_number,
                      const char* old_name, const char* new_name) {
    if (ctx == nullptr || old_name == nullptr || new_name == nullptr) return -1;

    const uint8_t new_len = static_cast<uint8_t>(strlen(new_name));
    if (new_len == 0) return -2;

    ext4_inode dir_inode;
    // Procura se o inode existe no old_name. Se não existir, retorna erro.
    if (ext4_get_inode(ctx, dir_inode_number, &dir_inode) != 0) return -3;

    off_t block_off = 0;
    uint32_t block_size = 0;
    // Obtém o primeiro bloco de dados do diretório via extents
    if (get_dir_first_block(ctx, &dir_inode, &block_off, &block_size) != 0) return -4;

    char* block = new char[block_size];
    // Lê o bloco de dados do diretório da imagem
    if (lseek(ctx->fd, block_off, SEEK_SET) < 0 ||
        read(ctx->fd, block, block_size) != block_size) {
        delete[] block;
        return -5;
    }

    
    uint32_t offset = 0;
    bool found = false;
    // Ate agora não entendi esse +8. As documentações não citam sobre
    while (offset + 8 <= block_size) {
        uint32_t inode = 0;
        uint16_t rec_len = 0;
        uint8_t name_len = 0;
        memcpy(&inode,    block + offset + 0, sizeof(uint32_t));
        memcpy(&rec_len,  block + offset + 4, sizeof(uint16_t));
        memcpy(&name_len, block + offset + 6, sizeof(uint8_t));

        if (rec_len < 8 || offset + rec_len > block_size) break;
        // Verifica se o nome atual bate com o old_name
        if (inode != 0 && name_len == strlen(old_name) &&
            memcmp(block + offset + 8, old_name, name_len) == 0) {
            // Verifica se o novo nome cabe (não pode ser maior que o espaço usado pelo nome atual)
            if (new_len > rec_len - 8) { delete[] block; return -6; }
            // Zera o nome antigo e escreve o novo
            memset(block + offset + 8, 0, rec_len - 8);
            memcpy(block + offset + 8, new_name, new_len);
            memcpy(block + offset + 6, &new_len, sizeof(uint8_t));
            found = true;
            break;
        }
        offset += rec_len;
    }

    // Se não encontrou o old_name no diretório, retorna erro
    if (!found) { delete[] block; return -7; }

    // Persiste o bloco modificado
    if (lseek(ctx->fd, block_off, SEEK_SET) < 0 ||
        write(ctx->fd, block, block_size) != block_size) {
        delete[] block;
        return -8;
    }
    delete[] block;
    return 0;
}

int ext4_add_dir_entry(ext4_context* ctx, uint32_t dir_inode_number,
                       uint32_t new_inode_number, const char* name, uint8_t file_type) {
    if (ctx == nullptr || name == nullptr) return -1;

    const uint8_t name_len = static_cast<uint8_t>(strlen(name));
    if (name_len == 0) return -2;
    // Tamanho mínimo necessário para a nova entrada
    const uint16_t needed = static_cast<uint16_t>((8 + name_len + 3) & ~3);

    ext4_inode dir_inode;
    // Procura se o inode do diretório existe. Se não existir, retorna erro.
    if (ext4_get_inode(ctx, dir_inode_number, &dir_inode) != 0) return -3;

    off_t block_off = 0;
    uint32_t block_size = 0;
    // Obtém o primeiro bloco de dados do diretório via extents
    if (get_dir_first_block(ctx, &dir_inode, &block_off, &block_size) != 0) return -4;

    char* block = new char[block_size];
    // Lê o bloco de dados do diretório da imagem
    if (lseek(ctx->fd, block_off, SEEK_SET) < 0 ||
        read(ctx->fd, block, block_size) != block_size) {
        delete[] block;
        return -5;
    }

    // Procura espaço livre: última entrada com rec_len maior do que o mínimo necessário
    uint32_t offset = 0;
    bool inserted = false;
    while (offset + 8 <= block_size) {
        uint16_t rec_len = 0;
        uint8_t  cur_name_len = 0;
        uint32_t cur_inode = 0;
        memcpy(&cur_inode,    block + offset + 0, sizeof(uint32_t));
        memcpy(&rec_len,      block + offset + 4, sizeof(uint16_t));
        memcpy(&cur_name_len, block + offset + 6, sizeof(uint8_t));

        // Bloco recém-zerado: cria a primeira entrada ocupando todo o bloco.
        if (offset == 0 && rec_len == 0) {
            const uint16_t full_len = static_cast<uint16_t>(block_size);
            memcpy(block + offset + 0, &new_inode_number, sizeof(uint32_t));
            memcpy(block + offset + 4, &full_len,         sizeof(uint16_t));
            memcpy(block + offset + 6, &name_len,         sizeof(uint8_t));
            memcpy(block + offset + 7, &file_type,        sizeof(uint8_t));
            memset(block + offset + 8, 0, block_size - 8);
            memcpy(block + offset + 8, name,              name_len);
            inserted = true;
            break;
        }

        if (rec_len < 8 || offset + rec_len > block_size) break;

        // Tamanho real que a entrada corrente ocupa (alinhado)
        const uint16_t actual = (cur_inode == 0)
            ? 0
            : static_cast<uint16_t>((8 + cur_name_len + 3) & ~3);
        const uint16_t free_space = rec_len - actual;

        if (free_space >= needed) {
            if (cur_inode != 0) {
                // Divide: encurta a entrada atual e coloca a nova no espaço restante
                const uint16_t new_offset = offset + actual;
                memcpy(block + offset + 4, &actual, sizeof(uint16_t)); // rec_len atual = seu tamanho mínimo

                const uint16_t new_rec_len = rec_len - actual;
                memcpy(block + new_offset + 0, &new_inode_number, sizeof(uint32_t));
                memcpy(block + new_offset + 4, &new_rec_len,        sizeof(uint16_t));
                memcpy(block + new_offset + 6, &name_len,           sizeof(uint8_t));
                memcpy(block + new_offset + 7, &file_type,          sizeof(uint8_t));
                memcpy(block + new_offset + 8, name,                name_len);
            } else {
                // Entrada deletada: reutiliza o slot inteiro
                memcpy(block + offset + 0, &new_inode_number, sizeof(uint32_t));
                memcpy(block + offset + 6, &name_len,         sizeof(uint8_t));
                memcpy(block + offset + 7, &file_type,        sizeof(uint8_t));
                memcpy(block + offset + 8, name,              name_len);
            }
            inserted = true;
            break;
        }
        offset += rec_len;
    }

    if (!inserted) { delete[] block; return -6; } // sem espaço no bloco

    if (lseek(ctx->fd, block_off, SEEK_SET) < 0 ||
        write(ctx->fd, block, block_size) != block_size) {
        delete[] block;
        return -7;
    }
    delete[] block;
    return 0;
}

int ext4_remove_dir_entry(ext4_context* ctx, uint32_t dir_inode_number, const char* name) {
    if (ctx == nullptr || name == nullptr) return -1;

    ext4_inode dir_inode;
    if (ext4_get_inode(ctx, dir_inode_number, &dir_inode) != 0) return -2;

    off_t block_off = 0;
    uint32_t block_size = 0;
    if (get_dir_first_block(ctx, &dir_inode, &block_off, &block_size) != 0) return -3;

    char* block = new char[block_size];
    if (lseek(ctx->fd, block_off, SEEK_SET) < 0 ||
        read(ctx->fd, block, block_size) != static_cast<ssize_t>(block_size)) {
        delete[] block;
        return -4;
    }

    uint32_t offset = 0;
    bool found = false;
    while (offset + 8 <= block_size) {
        uint32_t inode = 0;
        uint16_t rec_len = 0;
        uint8_t  name_len = 0;
        memcpy(&inode,    block + offset + 0, sizeof(uint32_t));
        memcpy(&rec_len,  block + offset + 4, sizeof(uint16_t));
        memcpy(&name_len, block + offset + 6, sizeof(uint8_t));

        if (rec_len < 8 || offset + rec_len > block_size) break;

        if (inode != 0 && name_len == strlen(name) &&
            memcmp(block + offset + 8, name, name_len) == 0) {
            // Forma inode como 0 para marcar como deletado
            memcpy(block + offset + 0, &ZERO, sizeof(uint32_t));
            found = true;
            break;
        }
        offset += rec_len;
    }

    if (!found) { delete[] block; return -5; }

    if (lseek(ctx->fd, block_off, SEEK_SET) < 0 ||
        write(ctx->fd, block, block_size) != block_size) {
        delete[] block;
        return -6;
    }
    delete[] block;
    return 0;
}
