#pragma once
#include <stdint.h>

// Metadados
// O inode descreve arquivos/diretórios e aponta para seus blocos de dados

#define TREEFS_MAX_INODES     64
#define TREEFS_DIRECT_BLOCKS   8
#define TREEFS_INVALID_BLOCK   0xFFFFFFFFU

#define TREEFS_TYPE_FREE       0
#define TREEFS_TYPE_FILE       1
#define TREEFS_TYPE_DIR        2

typedef struct inode
{
    uint32_t id;
    uint32_t type;
    uint32_t size;
    uint32_t refs;
    uint32_t block_count;
    uint32_t blocks[TREEFS_DIRECT_BLOCKS];
} inode_t;

void inode_table_init(void);
inode_t *inode_get(uint32_t inode);
inode_t *inode_alloc(void);
void inode_free(uint32_t inode);
uint32_t inode_used_count(void);