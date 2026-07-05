#pragma once

#include "../ext4/types.h"

// Faz o parse de um inode da tabela de inodes do EXT4.
int parse_inode(const char* block, struct ext4_inode* inode);
