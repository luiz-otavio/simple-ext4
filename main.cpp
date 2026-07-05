#include <iostream>
#include <cstring>

#include "ext4/context.h"
#include "shell/shell.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Uso: " << argv[0] << " <nome da imagem>" << std::endl;
        return 1;
    }

    ext4_context ctx;
    if (ext4_context_init(&ctx, argv[1]) != 0) {
        std::cerr << "Falha ao carregar a imagem do sistema de arquivos. (precisa ser um arquivo EXT4 válido)" << std::endl;
        return 1;
    }

    // Extrai só o nome do arquivo para o prompt
    const char* image_name = strrchr(argv[1], '/');
    image_name = (image_name != nullptr) ? image_name + 1 : argv[1];

    shell_run(&ctx, image_name);

    ext4_context_destroy(&ctx);
    return 0;
}
