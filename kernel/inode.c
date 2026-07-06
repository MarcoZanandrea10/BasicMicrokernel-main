#include "inode.h"
#include "string.h"

static inode_t inode_table[TREEFS_MAX_INODES];
static uint8_t inode_bitmap[TREEFS_MAX_INODES];

static void inode_reset(inode_t *node, uint32_t id)
{
    memset(node, 0, sizeof(inode_t));
    node->id = id;

    for (uint32_t i = 0; i < TREEFS_DIRECT_BLOCKS; i++)
        node->blocks[i] = TREEFS_INVALID_BLOCK;
}

void inode_table_init(void)
{
    memset(inode_bitmap, 0, sizeof(inode_bitmap));

    for (uint32_t i = 0; i < TREEFS_MAX_INODES; i++)
        inode_reset(&inode_table[i], i);
}

inode_t *inode_get(uint32_t inode)
{
    if (inode >= TREEFS_MAX_INODES || !inode_bitmap[inode])
        return 0;

    return &inode_table[inode];
}

inode_t *inode_alloc(void)
{
    for (uint32_t i = 0; i < TREEFS_MAX_INODES; i++)
    {
        if (!inode_bitmap[i])
        {
            inode_bitmap[i] = 1;
            inode_reset(&inode_table[i], i);
            inode_table[i].refs = 1;
            return &inode_table[i];
        }
    }

    return 0;
}

void inode_free(uint32_t inode)
{
    if (inode >= TREEFS_MAX_INODES)
        return;

    inode_bitmap[inode] = 0;
    inode_reset(&inode_table[inode], inode);
}

uint32_t inode_used_count(void)
{
    uint32_t used = 0;

    for (uint32_t i = 0; i < TREEFS_MAX_INODES; i++)
    {
        if (inode_bitmap[i])
            used++;
    }

    return used;
}
