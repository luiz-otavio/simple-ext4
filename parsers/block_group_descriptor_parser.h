#pragma once

#include "../ext4/types.h"

// Faz o parse de um descritor de grupo de blocos do EXT4.
int parse_block_group_descriptor(const char* block, struct ext4_block_group_descriptor* bgd);
