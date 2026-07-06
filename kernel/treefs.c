#include "treefs.h"
#include "uart.h"
#include "string.h"

#define DIR_CAPACITY (TREEFS_BLOCK_SIZE / sizeof(dir_entry_t))

static superblock_t filesystem_info;

static int copy_component_name(char *destination,
                               const char *source,
                               uint32_t length)
{
    if (length == 0 || length >= TREEFS_MAX_NAME)
        return -1;

    for (uint32_t index = 0; index < length; index++)
        destination[index] = source[index];

    destination[length] = 0;
    return 0;
}

/* Cada diretorio ocupa um bloco com entradas nome -> inode. */
static dir_entry_t *directory_entries(inode_t *directory_inode)
{
    if (!directory_inode ||
        directory_inode->type != TREEFS_TYPE_DIR ||
        directory_inode->block_count == 0)
        return 0;

    return (dir_entry_t*)block_get(directory_inode->blocks[0]);
}

static inode_t *find_entry(inode_t *directory_inode,
                           const char *entry_name,
                           uint32_t *entry_slot)
{
    dir_entry_t *entries = directory_entries(directory_inode);
    if (!entries)
        return 0;

    for (uint32_t entry_index = 0; entry_index < DIR_CAPACITY; entry_index++)
    {
        if (entries[entry_index].used &&
            strcmp(entries[entry_index].name, entry_name) == 0)
        {
            if (entry_slot)
                *entry_slot = entry_index;

            return inode_get(entries[entry_index].inode);
        }
    }

    return 0;
}

static int add_entry(inode_t *directory_inode,
                     const char *entry_name,
                     uint32_t inode_id)
{
    dir_entry_t *entries = directory_entries(directory_inode);

    if (!entries || find_entry(directory_inode, entry_name, 0))
        return -1;

    for (uint32_t entry_index = 0; entry_index < DIR_CAPACITY; entry_index++)
    {
        if (!entries[entry_index].used)
        {
            entries[entry_index].used = 1;
            entries[entry_index].inode = inode_id;
            strcpy(entries[entry_index].name, entry_name);
            directory_inode->size += sizeof(dir_entry_t);
            return 0;
        }
    }

    return -1;
}

static void remove_entry(inode_t *directory_inode, uint32_t entry_slot)
{
    dir_entry_t *entries = directory_entries(directory_inode);

    memset(&entries[entry_slot], 0, sizeof(dir_entry_t));

    if (directory_inode->size >= sizeof(dir_entry_t))
        directory_inode->size -= sizeof(dir_entry_t);
}

static int init_directory(inode_t *directory_inode)
{
    if (!directory_inode)
        return -1;

    int first_block = block_alloc();
    if (first_block < 0)
        return -1;

    directory_inode->type = TREEFS_TYPE_DIR;
    directory_inode->size = 0;
    directory_inode->block_count = 1;
    directory_inode->blocks[0] = (uint32_t)first_block;

    return 0;
}

static void free_inode_blocks(inode_t *file_inode)
{
    for (uint32_t block_index = 0;
         block_index < file_inode->block_count;
         block_index++)
        block_free(file_inode->blocks[block_index]);

    file_inode->size = 0;
    file_inode->block_count = 0;
}

/*
 * Resolve caminhos absolutos.
 * Quando final_name == 0, retorna o inode do caminho inteiro.
 * Quando final_name existe, retorna o diretorio pai e copia o ultimo nome.
 */
static inode_t *resolve_path(const char *path, char *final_name)
{
    if (!path || path[0] != '/')
        return 0;

    uint32_t path_length = strlen(path);
    if (path_length >= TREEFS_MAX_PATH ||
        (final_name && (path_length == 1 || path[path_length - 1] == '/')))
        return 0;

    inode_t *current_inode = inode_get(TREEFS_ROOT_INODE);
    if (!final_name && path[1] == 0)
        return current_inode;

    for (uint32_t cursor = 1; path[cursor];)
    {
        char component_name[TREEFS_MAX_NAME];
        uint32_t component_start = cursor;

        while (path[cursor] && path[cursor] != '/')
            cursor++;

        if (copy_component_name(component_name,
                                path + component_start,
                                cursor - component_start) != 0)
            return 0;

        while (path[cursor] == '/')
            cursor++;

        if (final_name && !path[cursor])
        {
            strcpy(final_name, component_name);
            return directory_entries(current_inode) ? current_inode : 0;
        }

        current_inode = find_entry(current_inode, component_name, 0);
        if (!current_inode)
            return 0;
    }

    return final_name ? 0 : current_inode;
}

static int create_node(const char *path, uint32_t node_type)
{
    /* mkdir() e create() usam o mesmo fluxo: achar pai, alocar inode e linkar. */
    char entry_name[TREEFS_MAX_NAME];
    inode_t *parent_directory = resolve_path(path, entry_name);

    if (!parent_directory || find_entry(parent_directory, entry_name, 0))
        return -1;

    inode_t *new_inode = inode_alloc();
    if (!new_inode)
        return -1;

    if (node_type == TREEFS_TYPE_DIR && init_directory(new_inode) != 0)
    {
        inode_free(new_inode->id);
        return -1;
    }

    if (node_type == TREEFS_TYPE_FILE)
        new_inode->type = TREEFS_TYPE_FILE;

    if (add_entry(parent_directory, entry_name, new_inode->id) != 0)
    {
        free_inode_blocks(new_inode);
        inode_free(new_inode->id);
        return -1;
    }

    return (int)new_inode->id;
}

int fs_init(void)
{
    filesystem_info.signature = TREEFS_SIGNATURE;
    filesystem_info.block_count = TREEFS_MAX_BLOCKS;
    filesystem_info.inode_count = TREEFS_MAX_INODES;
    filesystem_info.block_size = TREEFS_BLOCK_SIZE;

    block_init();
    inode_table_init();

    inode_t *root_inode = inode_alloc();
    if (!root_inode ||
        root_inode->id != TREEFS_ROOT_INODE ||
        init_directory(root_inode) != 0)
        return -1;

    return mkdir("/home") == 0 &&
           mkdir("/tmp") == 0 &&
           mkdir("/bin") == 0 ? 0 : -1;
}

inode_t *path_lookup(const char *path)
{
    return resolve_path(path, 0);
}

int mkdir(const char *path)
{
    return create_node(path, TREEFS_TYPE_DIR) < 0 ? -1 : 0;
}

/* Para simplificar, o fd retornado eh o proprio id do inode criado. */
int create(const char *path)
{
    return create_node(path, TREEFS_TYPE_FILE);
}

int unlink(const char *path)
{
    char entry_name[TREEFS_MAX_NAME];
    uint32_t entry_slot = 0;
    inode_t *parent_directory = resolve_path(path, entry_name);

    if (!parent_directory)
        return -1;

    inode_t *file_inode = find_entry(parent_directory, entry_name, &entry_slot);
    if (!file_inode || file_inode->type != TREEFS_TYPE_FILE)
        return -1;

    remove_entry(parent_directory, entry_slot);
    free_inode_blocks(file_inode);
    inode_free(file_inode->id);

    return 0;
}

int ls(const char *path)
{
    dir_entry_t *entries = directory_entries(path_lookup(path));
    int printed_entries = 0;

    if (!entries)
    {
        uart_print("ls: caminho invalido\n");
        return -1;
    }

    for (uint32_t entry_index = 0; entry_index < DIR_CAPACITY; entry_index++)
    {
        if (entries[entry_index].used)
        {
            uart_print(entries[entry_index].name);
            uart_print("\n");
            printed_entries++;
        }
    }

    return printed_entries;
}

int write(int fd, const void *buffer, uint32_t size)
{
    /* Escrita simples: substitui o conteudo inteiro do arquivo. */
    inode_t *file_inode = fd < 0 ? 0 : inode_get((uint32_t)fd);
    uint32_t needed_blocks = (size + TREEFS_BLOCK_SIZE - 1) / TREEFS_BLOCK_SIZE;

    if (!file_inode ||
        file_inode->type != TREEFS_TYPE_FILE ||
        (!buffer && size > 0) ||
        needed_blocks > TREEFS_DIRECT_BLOCKS ||
        block_free_count() + file_inode->block_count < needed_blocks)
        return -1;

    free_inode_blocks(file_inode);

    for (uint32_t block_index = 0; block_index < needed_blocks; block_index++)
    {
        uint32_t file_offset = block_index * TREEFS_BLOCK_SIZE;
        uint32_t bytes_left = size - file_offset;
        uint32_t chunk_size = bytes_left > TREEFS_BLOCK_SIZE
                            ? TREEFS_BLOCK_SIZE
                            : bytes_left;
        int allocated_block = block_alloc();

        file_inode->blocks[file_inode->block_count++] =
            (uint32_t)allocated_block;
        memcpy(block_get((uint32_t)allocated_block),
               (const uint8_t*)buffer + file_offset,
               chunk_size);
    }

    file_inode->size = size;
    return (int)size;
}

int read(int fd, void *buffer, uint32_t size)
{
    /* Leitura simples: copia desde o inicio ate size ou fim do arquivo. */
    inode_t *file_inode = fd < 0 ? 0 : inode_get((uint32_t)fd);
    uint32_t copied_bytes = 0;

    if (!file_inode ||
        file_inode->type != TREEFS_TYPE_FILE ||
        (!buffer && size > 0))
        return -1;

    if (size > file_inode->size)
        size = file_inode->size;

    for (uint32_t block_index = 0;
         block_index < file_inode->block_count && copied_bytes < size;
         block_index++)
    {
        uint32_t bytes_left = size - copied_bytes;
        uint32_t chunk_size = bytes_left > TREEFS_BLOCK_SIZE
                            ? TREEFS_BLOCK_SIZE
                            : bytes_left;

        memcpy((uint8_t*)buffer + copied_bytes,
               block_get(file_inode->blocks[block_index]),
               chunk_size);
        copied_bytes += chunk_size;
    }

    return (int)copied_bytes;
}

const superblock_t *treefs_superblock(void)
{
    return &filesystem_info;
}
