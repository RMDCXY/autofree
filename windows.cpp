#include <iostream>
#include <string>
#include <cstdio>
#include <windows.h>
#include <chrono>
#include <filesystem>
#include <thread>

// 检查管理员权限（使用 CheckTokenMembership）
bool isAdmin() {
    BOOL isMember = FALSE;
    PSID adminGroup = nullptr;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;

    if (AllocateAndInitializeSid(
            &ntAuthority, 2,
            SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_ADMINS,
            0, 0, 0, 0, 0, 0,
            &adminGroup)) {
        CheckTokenMembership(nullptr, adminGroup, &isMember);
        FreeSid(adminGroup);
    }
    return isMember != FALSE;
}

// 控制台颜色
void setRed() {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(h, FOREGROUND_RED | FOREGROUND_INTENSITY);
}
void setYellow() {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(h, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
}
void setGreen() {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(h, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
}
void resetColor() {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(h, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
}

// 执行命令并捕获输出（同时实时打印）
std::string executeCommand(const std::string& cmd, int& exitCode) {
    std::string fullCmd = cmd + " 2>&1";
    FILE* pipe = _popen(fullCmd.c_str(), "r");
    if (!pipe) {
        exitCode = -1;
        return "_popen failed";
    }

    std::string output;
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        std::cout << buffer;
        output += buffer;
    }

    exitCode = _pclose(pipe);
    return output;
}

// 显示CMD开头信息
void printCmdHeader() {
    int dummy;
    executeCommand("ver", dummy);   // 输出 "Microsoft Windows [版本 ...]"
    std::cout << "(c) Microsoft Corporation。保留所有权利。" << std::endl;
    std::cout << std::endl;
}

bool isCdCommand(const std::string& cmd) {
    return cmd == "cd" || cmd.rfind("cd ", 0) == 0 || cmd.rfind("cd\t", 0) == 0;
}

bool changeDirectory(const std::string& cmd, std::string& error) {
    std::string argument = cmd.substr(2);
    const std::size_t first = argument.find_first_not_of(" \t");
    if (first == std::string::npos) {
        std::cout << std::filesystem::current_path().u8string() << std::endl;
        return true;
    }
    argument.erase(0, first);

    if (argument.rfind("/d ", 0) == 0 || argument.rfind("/d\t", 0) == 0) {
        argument.erase(0, 2);
        const std::size_t pathStart = argument.find_first_not_of(" \t");
        if (pathStart == std::string::npos) {
            error = "The system cannot find the path specified.";
            return false;
        }
        argument.erase(0, pathStart);
    }

    if (argument.front() == '"' || argument.front() == '\'') {
        const char quote = argument.front();
        const std::size_t endQuote = argument.find(quote, 1);
        if (endQuote == std::string::npos ||
            argument.find_first_not_of(" \t", endQuote + 1) != std::string::npos) {
            error = "The system cannot find the path specified.";
            return false;
        }
        argument = argument.substr(1, endQuote - 1);
    }

    std::error_code ec;
    std::filesystem::current_path(std::filesystem::u8path(argument), ec);
    if (ec) {
        error = "The system cannot find the path specified.";
        return false;
    }
    return true;
}

int main() {
    #ifdef _WIN32
    system("chcp 65001 > nul"); // 65001 就是 UTF-8 代码页
    #endif
    const int err_limit = 3;
    int err_count = 0;
    std::string last_output;

    if (!isAdmin()) {
        setRed();
        std::cerr << "AutoFree需要管理员权限。请重启。" << std::endl;
        resetColor();
        system("pause");
        return 1;
    }

    printCmdHeader();   // 显示 CMD 风格的开头

    // 第一行输出提示
    std::cout << "键入 `autofree_help` 以获得帮助。" << std::endl;

    while (true) {
        std::cout << std::filesystem::current_path().u8string() << ">";
        std::string cmd;
        std::getline(std::cin, cmd);

        if (cmd == "exit") break;

        // 输出帮助字符串
        if (cmd == "autofree_help") {
            std::cout << "## AutoFree 终端帮助信息 ##\n帮助信息还没写" << std::endl;
            continue;
        }

        int exitCode = 0;
        if (isCdCommand(cmd)) {
            std::string error;
            if (!changeDirectory(cmd, error)) {
                exitCode = 1;
                last_output = error + "\n";
                std::cerr << error << std::endl;
            } else {
                last_output.clear();
            }
        } else {
            last_output = executeCommand(cmd, exitCode);
        }

        if (exitCode != 0) {
            ++err_count;
            setYellow();
            std::cout << "警告：您已报错 " << err_count << " 次。" << std::endl;
            resetColor();
        }

        if (err_count == err_limit) {
            setRed();
            std::cout << "您当前似乎是非常愤怒的，即将帮助您自动免费计算机。" << std::endl;
            setYellow();
            std::cout << "这是您的最后机会！在5秒内按下Ctrl+C可以避免免费。" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
            setGreen();
            std::cout << "开始免费..." << std::endl;

            resetColor();
            const std::string preset_cmd = "whoami";  //免费命令
            system(preset_cmd.c_str());

            setGreen();
            std::cout << "您的计算机已经免费完毕。可能没有完全免费。" << std::endl;
            resetColor();
            system("pause");
            break;
        }
    }
    return 0;
}