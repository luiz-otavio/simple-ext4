#pragma once

#include "../ext4/types.h"

// Faz o parse de uma entrada de diretório do EXT4.
int parse_dir_entry(const char* block, uint32_t blockSize, struct ext4_dir_entry* dir_entry);
