#pragma once

#include <cstdint>
#include "../ext4/context.h"

void cmd_info(ext4_context* ctx); // Comando 'info' - mostra informações do superbloco e do contexto
void cmd_ls(ext4_context* ctx); // Comando 'ls' - lista arquivos e diretórios do diretório corrente
void cmd_pwd(ext4_context* ctx); // Comando 'pwd' - mostra o caminho absoluto do diretório corrente
void cmd_cd(ext4_context* ctx, const char* path); // Comando 'cd' - muda o diretório corrente para o caminho especificado
void cmd_cat(ext4_context* ctx, const char* path); // Comando 'cat' - mostra o conteúdo de um arquivo especificado pelo caminho
void cmd_attr(ext4_context* ctx, const char* path); // Comando 'attr' - mostra os atributos do arquivo ou diretório especificado pelo caminho
void cmd_testi(ext4_context* ctx, uint32_t inode_number); // Comando 'testi' - mostra informações detalhadas do inode especificado pelo número
void cmd_testb(ext4_context* ctx, uint32_t block_number); // Comando 'testb' - mostra informações detalhadas do bloco especificado pelo número
void cmd_export(ext4_context* ctx, const char* source, const char* dest); // Comando 'export' - exporta um arquivo do sistema de arquivos EXT4 para o sistema de arquivos do host
