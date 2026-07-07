#pragma once
#include <stdint.h>

// Dados
// Define os blocos onde o conteúdo real dos arquivos é armazenado

#define TREEFS_BLOCK_SIZE      256
#define TREEFS_MAX_BLOCKS      128

void block_init(void);
int block_alloc(void);
void block_free(uint32_t block);
void *block_get(uint32_t block);
uint32_t block_used_count(void);
uint32_t block_free_count(void);