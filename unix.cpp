#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>
#include <chrono>
#include <thread>
#include <limits.h>

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

bool isCdCommand(const std::string& cmd) {
    return cmd == "cd" || cmd.rfind("cd ", 0) == 0 || cmd.rfind("cd\t", 0) == 0;
}

bool changeDirectory(const std::string& cmd, std::filesystem::path& previousDirectory,
                     std::string& error) {
    std::string argument = cmd.substr(2);
    const std::size_t first = argument.find_first_not_of(" \t");
    std::filesystem::path target;
    if (first == std::string::npos) {
        const char* home = std::getenv("HOME");
        if (home == nullptr || *home == '\0') {
            error = "cd: HOME not set";
            return false;
        }
        target = std::filesystem::u8path(home);
    } else {
        argument.erase(0, first);
        if (argument.front() == '"' || argument.front() == '\'') {
            const char quote = argument.front();
            const std::size_t endQuote = argument.find(quote, 1);
            if (endQuote == std::string::npos ||
                argument.find_first_not_of(" \t", endQuote + 1) != std::string::npos) {
                error = "cd: invalid path";
                return false;
            }
            argument = argument.substr(1, endQuote - 1);
        }

        if (argument == "-") {
            if (previousDirectory.empty()) {
                error = "cd: OLDPWD not set";
                return false;
            }
            target = previousDirectory;
        } else {
            const char* home = std::getenv("HOME");
            if (home != nullptr &&
                (argument == "~" || argument.rfind("~/", 0) == 0)) {
                argument.replace(0, 1, home);
            }
            target = std::filesystem::u8path(argument);
        }
    }

    std::error_code ec;
    const std::filesystem::path oldDirectory = std::filesystem::current_path(ec);
    if (ec) {
        error = "cd: " + ec.message();
        return false;
    }
    std::filesystem::current_path(target, ec);
    if (ec) {
        error = "cd: " + ec.message();
        return false;
    }
    previousDirectory = oldDirectory;
    if (argument == "-" && first != std::string::npos) {
        std::cout << std::filesystem::current_path().u8string() << std::endl;
    }
    return true;
}

void printPrompt(const std::filesystem::path& homeDirectory) {
    const passwd* userInfo = getpwuid(geteuid());
    const char* userEnv = std::getenv("USER");
    const std::string username = userEnv != nullptr && *userEnv != '\0'
        ? userEnv
        : (userInfo != nullptr ? userInfo->pw_name : "user");
    char hostname[HOST_NAME_MAX + 1] = {};
    if (gethostname(hostname, sizeof(hostname) - 1) != 0) {
        hostname[0] = '\0';
    }

    std::error_code ec;
    std::filesystem::path currentDirectory = std::filesystem::current_path(ec);
    std::string displayedDirectory = ec ? "?" : currentDirectory.u8string();
    const std::string home = homeDirectory.u8string();
    if (!home.empty() && displayedDirectory == home) {
        displayedDirectory = "~";
    } else if (!home.empty() && displayedDirectory.rfind(home + "/", 0) == 0) {
        displayedDirectory.replace(0, home.size(), "~");
    }

    std::cout << "\033[1;32m" << username << "@" << hostname
              << "\033[0m:\033[1;94m" << displayedDirectory
              << "\033[0m$ ";
}

int main() {
    const int err_limit = 3;
    int err_count = 0;
    std::string last_output;
    std::filesystem::path previousDirectory;
    const char* home = std::getenv("HOME");
    const std::filesystem::path homeDirectory = home != nullptr ? std::filesystem::u8path(home) : "";

    // 权限检查
    if (!hasRootPrivilege()) {
        std::cerr << "\033[31mAutoFree需要以root权限运行。请重启。\033[0m" << std::endl;
        return 1;
    }

    // 第一行输出提示
    std::cout << "键入 `autofree_help` 以获得帮助。" << std::endl;

    while (true) {
        printPrompt(homeDirectory);
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
        if (isCdCommand(cmd)) {
            std::string error;
            if (!changeDirectory(cmd, previousDirectory, error)) {
                exitCode = 1;
                last_output = error + "\n";
                std::cerr << error << std::endl;
            } else {
                last_output.clear();
            }
        } else {
            last_output = executeCommand(cmd, exitCode);
        }

        // 命令报错（退出码非0）
        if (exitCode != 0) {
            ++err_count;
            std::cout << "\033[33m警告：您已报错 " << err_count << " 次。\033[0m" << std::endl;
        }

        // 免费逻辑
        if (err_count == err_limit) {
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
