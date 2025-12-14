#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cstdlib>

namespace fs = std::filesystem;

std::string currentDir = fs::current_path().string();

void clearScreen() {
    std::cout << "\x1b[2J\x1b[H";
}

void setTerminalTitle(const std::string &title) {
    std::cout << "\x1b]0;" << title << "\x07";
}

// simple getch() for POSIX
int getch_posix() {
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    unsigned char c;
    int n = (int)read(STDIN_FILENO, &c, 1);
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    if (n <= 0) return 0;
    return c;
}

// Autocomplete helper
std::vector<std::string> getMatchingDirectories(const std::string& prefix) {
    std::vector<std::string> matches;
    for (const auto& entry : fs::directory_iterator(currentDir)) {
        if (entry.is_directory()) {
            std::string name = entry.path().filename().string();
            if (name.find(prefix) == 0) {
                matches.push_back(name);
            }
        }
    }
    return matches;
}

void processCommand() {
    std::string buffer;

    while (true) {
        std::string prompt = currentDir + ">> ";
        std::cout << prompt;
        std::cout.flush();
        buffer.clear();

        while (true) {
            int ich = getch_posix();
            char ch = (char)ich;

            if (ch == '\r' || ch == '\n') { // ENTER
                std::cout << "\n";
                break;
            } else if (ich == 127 || ch == '\b') { // BACKSPACE (DEL or BS)
                if (!buffer.empty()) {
                    buffer.pop_back();
                    std::cout << "\b \b" << std::flush;
                }
            } else if (ch == '\t') { // TAB (autocomplete)
                if (buffer.rfind("cd ", 0) == 0) {
                    std::string partial = buffer.substr(3);
                    auto matches = getMatchingDirectories(partial);
                    if (!matches.empty()) {
                        buffer = "cd " + matches[0];
                        // Přepiš aktuální řádek
                        std::cout << "\r\x1b[K" << prompt << buffer << std::flush;
                    }
                }
            } else {
                buffer += ch;
                std::cout << ch << std::flush;
            }
        }

        const std::string& command = buffer;

        if (command == "help") {
            std::cout << "The commands are:\n";
            std::cout << "print 'example' - Prints out text\n";
            std::cout << "exec 'cmd'     - Executes a command\n";
            std::cout << "cd DIR         - Changes directory\n";
            std::cout << "dir / ls       - Lists current directory\n";
            std::cout << "nano / mkfile  - Runs text editor\n";
            std::cout << "cls / clear    - Clears the screen\n";
            std::cout << "ssh            - Runs ssh client\n";
            std::cout << "exit           - Quits the program\n";
        }
        else if (command == "exit") {
            std::cout << "Exiting...\n";
            break;
        }
        else if (command == "cls" || command == "clear") {
            clearScreen();
        }
        else if (command == "dir" || command == "ls") {
            std::string cmd = "ls -la \"" + currentDir + "\"";
            system(cmd.c_str());
        }
        else if (command == "nano" || command == "mkfile") {
            int result = system("./bin/nano");
            clearScreen(); // Clear screen after nano
            setTerminalTitle("FastBox");
            if (result == -1) std::cout << "Failed to start nano.\n";
        }
        else if (command == "ssh") {
            int result = system("./bin/ssh");
            if (result == -1) std::cout << "Failed to start ssh client.\n";
            clearScreen(); // Clear screen after ssh client closes
            setTerminalTitle("FastBox");
        }
        else if (command.rfind("cd ", 0) == 0) {
            std::string path = command.substr(3);
            fs::path newPath = currentDir;
            newPath /= path;
            if (fs::exists(newPath) && fs::is_directory(newPath)) {
                currentDir = fs::canonical(newPath).string();
                chdir(currentDir.c_str());
            } else {
                std::cout << "Directory not found: " << path << "\n";
            }
        }
        else if (command.rfind("print ", 0) == 0) {
            size_t first = command.find('\'');
            size_t last = command.rfind('\'');
            if (first != std::string::npos && last != std::string::npos && first != last) {
                std::string msg = command.substr(first + 1, last - first - 1);
                std::cout << msg << "\n";
            } else {
                std::cout << "Error: Missing quotes.\n";
            }
        }
        else if (command.rfind("exec ", 0) == 0) {
            size_t first = command.find('\'');
            size_t last = command.rfind('\'');
            if (first != std::string::npos && last != std::string::npos && first != last) {
                std::string cmd = command.substr(first + 1, last - first - 1);
                int result = system(cmd.c_str());
                clearScreen(); // Clear after external command
                setTerminalTitle("FastBox");                
                if (result == -1) std::cout << "Failed to run.\n";
            } else {
                std::cout << "Error: Missing quotes.\n";
            }
        }
        else {
            std::cout << "Unknown command. Type 'help'.\n";
        }
    }
}

int main() {
    clearScreen();
    setTerminalTitle("FastBox");
    std::cout << "Welcome to FastBox - Version 1.0\n";
    std::cout << "Make your own FastBox! Download source code at: https://www.github.com/MatyysLinux/FastBox\n";
    std::cout << "Owned by MatyysLinux!\n\n";
    setTerminalTitle("FastBox");
    processCommand();
    setTerminalTitle("FastBox");
    return 0;
}
