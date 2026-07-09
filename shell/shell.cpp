#include <iostream>
#include <string>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "shell.h"
#include "commands_read.h"
#include "commands_write.h"

static std::vector<std::string> split_args(const std::string& line) {
    std::vector<std::string> args;
    std::istringstream ss(line);
    std::string token;
    while (ss >> token) args.push_back(token);
    return args;
}

void shell_run(ext4_context* ctx, const char* image_name) {
    std::string line;

    while (true) {
        std::cout << "ext4shell:[" << image_name << ctx->current_path << "] $ ";
        if (!std::getline(std::cin, line)) break;
        if (line.empty()) continue;

        const std::vector<std::string> args = split_args(line);
        const std::string& cmd = args[0];

        if (cmd == "exit" || cmd == "quit") {
            ext4_context_save(ctx);
            break;
        } else if (cmd == "info") {
            cmd_info(ctx);
        } else if (cmd == "ls") {
            cmd_ls(ctx);
        } else if (cmd == "pwd") {
            cmd_pwd(ctx);
        } else if (cmd == "cd") {
            const char* path = (args.size() >= 2) ? args[1].c_str() : "/";
            cmd_cd(ctx, path);
        } else if (cmd == "cat") {
            if (args.size() < 2) { std::cerr << "Uso: cat <file>" << std::endl; continue; }
            cmd_cat(ctx, args[1].c_str());
        } else if (cmd == "attr") {
            if (args.size() < 2) { std::cerr << "Uso: attr <file|dir>" << std::endl; continue; }
            cmd_attr(ctx, args[1].c_str());
        } else if (cmd == "testi") {
            if (args.size() < 2) { std::cerr << "Uso: testi <inode_number>" << std::endl; continue; }
            try {
                size_t pos = 0;
                const unsigned long value = std::stoul(args[1], &pos);
                if (pos != args[1].size()) {
                    std::cerr << "testi: valor inválido: " << args[1] << std::endl;
                    continue;
                }
                cmd_testi(ctx, static_cast<uint32_t>(value));
            } catch (const std::exception&) {
                std::cerr << "testi: valor inválido: " << args[1] << std::endl;
                continue;
            }
        } else if (cmd == "testb") {
            if (args.size() < 2) { std::cerr << "Uso: testb <block_number>" << std::endl; continue; }
            try {
                size_t pos = 0;
                const unsigned long value = std::stoul(args[1], &pos);
                if (pos != args[1].size()) {
                    std::cerr << "testb: valor inválido: " << args[1] << std::endl;
                    continue;
                }
                cmd_testb(ctx, static_cast<uint32_t>(value));
            } catch (const std::exception&) {
                std::cerr << "testb: valor inválido: " << args[1] << std::endl;
                continue;
            }
        } else if (cmd == "export") {
            if (args.size() < 3) { std::cerr << "Uso: export <source> <dest>" << std::endl; continue; }
            cmd_export(ctx, args[1].c_str(), args[2].c_str());
        } else if (cmd == "touch") {
            if (args.size() < 2) { std::cerr << "Uso: touch <file>" << std::endl; continue; }
            cmd_touch(ctx, args[1].c_str());
        } else if (cmd == "mkdir") {
            if (args.size() < 2) { std::cerr << "Uso: mkdir <dir>" << std::endl; continue; }
            cmd_mkdir(ctx, args[1].c_str());
        } else if (cmd == "rm") {
            if (args.size() < 2) { std::cerr << "Uso: rm <file>" << std::endl; continue; }
            cmd_rm(ctx, args[1].c_str());
        } else if (cmd == "rmdir") {
            if (args.size() < 2) { std::cerr << "Uso: rmdir <dir>" << std::endl; continue; }
            cmd_rmdir(ctx, args[1].c_str());
        } else if (cmd == "rename") {
            if (args.size() < 3) { std::cerr << "Uso: rename <file> <newname>" << std::endl; continue; }
            cmd_rename(ctx, args[1].c_str(), args[2].c_str());
        } else {
            std::cerr << "Comando desconhecido: " << cmd << std::endl;
        }
    }
}
