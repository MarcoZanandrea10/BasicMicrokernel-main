#include "memory.h"
#include "uart.h"
#include "treefs.h"
#include "string.h"

extern void uart_print(const char*);

// Função auxiliar para mostrar se cada operação do TreeFS funcionou ou deu erro
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
    memory_init(); // Inicia memória do microkernel antes de iniciar os módulos

    uart_print("\n=== Kernel TreeFS ===\n");

    // Integra o TreeFS ao kernel inicializando o sistema de arquivos
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

    // Testa a listagem da raiz criada pelo fs_init
    uart_print("\nCenario 1 - ls(\"/\"):\n");
    ls("/");

    // Testa a criação de um diretório dentro de /home
    print_status("\nCenario 2 - mkdir(\"/home/aluno\")",
                 mkdir("/home/aluno"));

    // Testa a criação de um arquivo e usa o inode retornado como fd
    int fd = create("/home/aluno/notas.txt");
    print_status("Cenario 3 - create(\"/home/aluno/notas.txt\")", fd);

    // Testa a escrita de dados no arquivo criado
    const char *text = "Sistemas Operacionais";
    print_status("Cenario 4 - write(fd, \"Sistemas Operacionais\", 22)",
                 write(fd, text, 22));

    char buffer[64];
    memset(buffer, 0, sizeof(buffer));

    // Testa a leitura dos dados gravados no arquivo
    int bytes = read(fd, buffer, 22);
    if (bytes >= 0 && bytes < (int)sizeof(buffer))
        buffer[bytes] = 0;

    print_status("Cenario 5 - read(fd, buffer, 22)", bytes);
    uart_print("Conteudo lido: ");
    uart_print(buffer);
    uart_print("\n");

    // Guarda o inode e o bloco usados antes da remoção para testar reutilização
    inode_t *notas = path_lookup("/home/aluno/notas.txt");
    uint32_t old_inode = notas ? notas->id : TREEFS_INVALID_BLOCK;
    uint32_t old_block = (notas && notas->block_count > 0)
                       ? notas->blocks[0]
                       : TREEFS_INVALID_BLOCK;

    // Testa a remoção do arquivo e liberação de inode/bloco
    print_status("Cenario 6 - unlink(\"/home/aluno/notas.txt\")",
                 unlink("/home/aluno/notas.txt"));

    // Testa a navegação hierárquica do conteúdo de /home
    uart_print("\nCenario 7 - ls(\"/home\"):\n");
    ls("/home");

    // Cria outro arquivo para verificar se inode e bloco liberados serão reutilizados
    int fd2 = create("/home/aluno/reuso.txt");
    write(fd2, "Reuso", 6);

    inode_t *reuso = path_lookup("/home/aluno/reuso.txt");
    uint32_t new_inode = reuso ? reuso->id : TREEFS_INVALID_BLOCK;
    uint32_t new_block = (reuso && reuso->block_count > 0)
                       ? reuso->blocks[0]
                       : TREEFS_INVALID_BLOCK;

    // Confirma se o gerenciamento reutilizou o mesmo inode e bloco liberados
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

    while (1); // Mantém executando
}