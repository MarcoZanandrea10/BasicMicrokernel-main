#include "block.h"
#include "string.h"

// Bitmap controla quais blocos estão livres ou ocupados
// Disco virtual guarda os dados reais dos arquivos

static uint8_t block_bitmap[TREEFS_MAX_BLOCKS];
static uint8_t virtual_disk[TREEFS_MAX_BLOCKS][TREEFS_BLOCK_SIZE];

// Inicia os blocos como livres e limpa o disco virtual
void block_init(void)
{
    memset(block_bitmap, 0, sizeof(block_bitmap));
    memset(virtual_disk, 0, sizeof(virtual_disk));
}

// Procura o primeiro bloco livre, marca como ocupado e retorna seu índice
int block_alloc(void)
{
    for (uint32_t i = 0; i < TREEFS_MAX_BLOCKS; i++)
    {
        if (!block_bitmap[i])
        {
            block_bitmap[i] = 1;
            memset(virtual_disk[i], 0, TREEFS_BLOCK_SIZE); // Limpa antes de reutilizar
            return (int)i;
        }
    }

    return -1;
}

// Libera um bloco ocupado para reutilizar
void block_free(uint32_t block)
{
    if (block >= TREEFS_MAX_BLOCKS)
        return;

    block_bitmap[block] = 0;
    memset(virtual_disk[block], 0, TREEFS_BLOCK_SIZE); // Apaga os dados
}

// Retorna o endereço do bloco no disco virtual
void *block_get(uint32_t block)
{
    if (block >= TREEFS_MAX_BLOCKS || !block_bitmap[block])
        return 0;

    return virtual_disk[block];
}

// Conta quantos blocos estão ocupados
uint32_t block_used_count(void)
{
    uint32_t used = 0;

    for (uint32_t i = 0; i < TREEFS_MAX_BLOCKS; i++)
    {
        if (block_bitmap[i])
            used++;
    }

    return used;
}

// Calcula quantos blocos estão livres
uint32_t block_free_count(void)
{
    return TREEFS_MAX_BLOCKS - block_used_count();
}
