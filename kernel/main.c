#include "memory.h"
#include "uart.h"
#include "treefs.h"
#include "string.h"

extern void uart_print(const char*);

static void print_status(const char *label, int result)
{
    uart_print(label);
    uart_print(": ");

    if (result >= 0)
    {
        uart_print("OK");
        if (result > 0)
        {
            uart_print(" (");
            uart_print_uint((uint64_t)result);
            uart_print(")");
        }
    }
    else
    {
        uart_print("ERRO");
    }

    uart_print("\n");
}

void kernel_main()
{
    memory_init();

    uart_print("\n=== Kernel TreeFS ===\n");

    int result = fs_init();
    print_status("fs_init", result);

    if (result != 0)
    {
        uart_print("Falha ao inicializar TreeFS\n");
        while (1);
    }

    const superblock_t *sb = treefs_superblock();
    uart_print("Superblock: blocos=");
    uart_print_uint(sb->block_count);
    uart_print(" inodes=");
    uart_print_uint(sb->inode_count);
    uart_print(" bloco_size=");
    uart_print_uint(sb->block_size);
    uart_print("\n");

    uart_print("\nCenario 1 - ls(\"/\"):\n");
    ls("/");

    print_status("\nCenario 2 - mkdir(\"/home/aluno\")",
                 mkdir("/home/aluno"));

    int fd = create("/home/aluno/notas.txt");
    print_status("Cenario 3 - create(\"/home/aluno/notas.txt\")", fd);

    const char *text = "Sistemas Operacionais";
    print_status("Cenario 4 - write(fd, \"Sistemas Operacionais\", 22)",
                 write(fd, text, 22));

    char buffer[64];
    memset(buffer, 0, sizeof(buffer));

    int bytes = read(fd, buffer, 22);
    if (bytes >= 0 && bytes < (int)sizeof(buffer))
        buffer[bytes] = 0;

    print_status("Cenario 5 - read(fd, buffer, 22)", bytes);
    uart_print("Conteudo lido: ");
    uart_print(buffer);
    uart_print("\n");

    inode_t *notas = path_lookup("/home/aluno/notas.txt");
    uint32_t old_inode = notas ? notas->id : TREEFS_INVALID_BLOCK;
    uint32_t old_block = (notas && notas->block_count > 0)
                       ? notas->blocks[0]
                       : TREEFS_INVALID_BLOCK;

    print_status("Cenario 6 - unlink(\"/home/aluno/notas.txt\")",
                 unlink("/home/aluno/notas.txt"));

    uart_print("\nCenario 7 - ls(\"/home\"):\n");
    ls("/home");

    int fd2 = create("/home/aluno/reuso.txt");
    write(fd2, "Reuso", 6);

    inode_t *reuso = path_lookup("/home/aluno/reuso.txt");
    uint32_t new_inode = reuso ? reuso->id : TREEFS_INVALID_BLOCK;
    uint32_t new_block = (reuso && reuso->block_count > 0)
                       ? reuso->blocks[0]
                       : TREEFS_INVALID_BLOCK;

    uart_print("\nCenario 8 - reutilizacao de inode/bloco: ");
    if (old_inode == new_inode && old_block == new_block)
        uart_print("OK\n");
    else
        uart_print("ERRO\n");

    uart_print("Inode antigo=");
    uart_print_uint(old_inode);
    uart_print(" novo=");
    uart_print_uint(new_inode);
    uart_print(" | Bloco antigo=");
    uart_print_uint(old_block);
    uart_print(" novo=");
    uart_print_uint(new_block);
    uart_print("\n");

    uart_print("\nTreeFS pronto.\n");

    while (1);
}
