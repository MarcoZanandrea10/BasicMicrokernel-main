#pragma once
#include <stdint.h>
#include "inode.h"
#include "block.h"

#define TREEFS_SIGNATURE       0x54524653U
#define TREEFS_MAX_NAME        24
#define TREEFS_MAX_PATH        128
#define TREEFS_ROOT_INODE      0

typedef struct
{
    uint32_t signature;
    uint32_t block_count;
    uint32_t inode_count;
    uint32_t block_size;
} superblock_t;

typedef struct
{
    uint8_t used;
    uint32_t inode;
    char name[TREEFS_MAX_NAME];
} dir_entry_t;

int fs_init(void);
inode_t *path_lookup(const char *path);
int mkdir(const char *path);
int create(const char *path);
int unlink(const char *path);
int ls(const char *path);
int read(int fd, void *buf, uint32_t size);
int write(int fd, const void *buf, uint32_t size);

const superblock_t *treefs_superblock(void);
