#pragma once

#include "../ext4/types.h"

// Faz o parse de um superbloco do EXT4.
// OBS: Fiquei super feliz quando isso aqui funcionou
// Parecia que eu estava mexendo em um disco de verdade, e não em um arquivo de imagem
int parse_superblock(const char* block, struct ext4_superblock* superblock);

// Escreve um superbloco do EXT4 em um bloco de bytes.
int write_superblock(const struct ext4_superblock* superblock, char* block);