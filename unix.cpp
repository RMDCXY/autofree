#include <iostream>
#include <string>
#include <cstdio>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstdlib>
#include <chrono>
#include <thread>

// 检查 root 权限
bool hasRootPrivilege() {
    return (geteuid() == 0);
}

// 执行命令并捕获输出（同时实时打印）
std::string executeCommand(const std::string& cmd, int& exitCode) {
    std::string fullCmd = cmd + " 2>&1";
    FILE* pipe = popen(fullCmd.c_str(), "r");
    if (!pipe) {
        exitCode = -1;
        return "popen failed";
    }

    std::string output;
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        std::cout << buffer;
        output += buffer;
    }

    exitCode = pclose(pipe);
    if (exitCode != -1 && WIFEXITED(exitCode)) {
        exitCode = WEXITSTATUS(exitCode);
    } else {
        exitCode = -1;
    }
    return output;
}

int main() {
    const int err_limit = 3;
    int err_count = 0;
    std::string last_output;

    // 权限检查
    if (!hasRootPrivilege()) {
        std::cerr << "\033[31mAutoFree需要以root权限运行。请重启。\033[0m" << std::endl;
        return 1;
    }

    // 第一行输出提示
    std::cout << "键入 `autofree_help` 以获得帮助。" << std::endl;

    while (true) {
        std::cout << "\033[32m$ \033[0m";   // 绿色提示符
        std::string cmd;
        std::getline(std::cin, cmd);

        if (cmd == "exit") break;

        // 输出帮助字符串
        if (cmd == "autofree_help") {
            std::cout << "## AutoFree 终端帮助信息 ##\n帮助信息还没写" << std::endl;
            continue;
        }

        // 执行普通命令
        int exitCode = 0;
        last_output = executeCommand(cmd, exitCode);

        // 命令报错（退出码非0）
        if (exitCode != 0) {
            ++err_count;
            std::cout << "\033[33m警告：您已报错 " << err_count << " 次。\033[0m" << std::endl;
        }

        // 免费逻辑
        if (err_count == err_limit) {
            std::cout << last_output;
            std::cout << "\033[31m您当前似乎是非常愤怒的，即将帮助您自动免费计算机。\033[0m" << std::endl;
            std::cout << "\033[33m这是您的最后机会！在3秒内按下Ctrl+C可以避免免费。\033[0m" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(3));

            std::cout << "\033[32m开始免费...\033[0m" << std::endl;
            const std::string preset_cmd = "whoami";  //免费命令
            system(preset_cmd.c_str());

            std::cout << "\033[32m您的计算机已经免费完毕。可能没有完全免费。\033[0m" << std::endl;
            break;
        }
    }
    return 0;
}
