#pragma once

#include "../ext4/context.h"

void cmd_touch(ext4_context* ctx, const char* name); // Comando TOUCH que cria um arquivo vazio com o nome especificado
void cmd_mkdir(ext4_context* ctx, const char* name); // Comando MKDIR que cria um diretório com o nome especificado
void cmd_rm(ext4_context* ctx, const char* name); // Comando RM que remove um arquivo com o nome especificado
void cmd_rmdir(ext4_context* ctx, const char* name); // Comando RMDIR que remove um diretório vazio com o nome especificado
void cmd_rename(ext4_context* ctx, const char* name, const char* newname); // Comando RENAME que renomeia um arquivo ou diretório de 'name' para 'newname'
