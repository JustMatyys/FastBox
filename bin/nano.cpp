#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

// Nastaví pozici kurzoru v konzoli (0-based coords)
void setCursorPosition(int x, int y) {
    // ANSI escape: row;col are 1-based
    std::cout << "\x1b[" << (y + 1) << ";" << (x + 1) << "H";
}

// Vymaže konzoli
void clearScreen() {
    std::cout << "\x1b[2J\x1b[H";
}

// Simple console messages instead of GUI popups
void showMessage(const std::string& text, const std::string& title = "Info") {
    std::cout << "[" << title << "] " << text << "\n";
}

void showError(const std::string& text, const std::string& title = "Error") {
    std::cerr << "[" << title << "] " << text << "\n";
}

// Prompt for filename on console (no GUI)
std::string promptFileName(const std::string& prompt) {
    std::string filename;
    std::cout << prompt << " (empty = cancel): ";
    std::getline(std::cin, filename);
    return filename;
}

// Uloží obsah lines do souboru
void saveToFile(const std::vector<std::string>& lines) {
    std::string filename = promptFileName("Save as");
    if (filename.empty()) {
        showMessage("Saving stopped.");
        return;
    }
    std::ofstream ofs(filename);
    if (!ofs) {
        showError("Error while opening file.");
        return;
    }
    for (const auto& line : lines) {
        ofs << line << "\n";
    }
    ofs.close();
    showMessage("File has been saved: " + filename);
}

// Načte obsah souboru do lines
bool loadFromFile(std::vector<std::string>& lines, int& curLine, int& curPos) {
    std::string filename = promptFileName("Open file");
    if (filename.empty()) {
        showMessage("Loading stopped.");
        return false;
    }
    std::ifstream ifs(filename);
    if (!ifs) {
        showError("Loading failed. Cant open file.");
        return false;
    }
    lines.clear();
    std::string line;
    while (std::getline(ifs, line)) {
        lines.push_back(line);
    }
    if (lines.empty()) lines.push_back("");
    curLine = 0;
    curPos = 0;
    ifs.close();
    showMessage("File loaded: " + filename);
    return true;
}

// Vykreslí všechny řádky s čísly řádků a kurzor na správném místě
void renderText(const std::vector<std::string>& lines, int curLine, int curPos) {
    clearScreen();

    // Hlavička
    std::cout << "FastBox Text Editor (Ctrl+S save, Ctrl+O open, ESC exit)\n";
    std::cout << "-----------------------------------------------------------\n";

    int lineNumberWidth = std::to_string(lines.size()).length();

    for (size_t i = 0; i < lines.size(); ++i) {
        // Formátuj číslo řádku na pevnou šířku (vpravo zarovnané)
        std::cout.width(lineNumberWidth);
        std::cout << (i + 1) << ". " << lines[i] << "\n";
    }

    // Nastav kurzor na správnou pozici (za číslo řádku + ". " + posun v textu)
    int cursorX = lineNumberWidth + 2 + curPos;
    int cursorY = 2 + curLine;

    setCursorPosition(cursorX, cursorY);
    std::cout.flush();
}

// Editor s klávesnicí a ovládáním
void runEditor() {
    std::vector<std::string> lines = {""};
    int curLine = 0;
    int curPos = 0;

    renderText(lines, curLine, curPos);

    // Helper to read keys including arrow sequences
    auto readKey = []() -> int {
        struct termios oldt, newt;
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);

        unsigned char c = 0;
        if (read(STDIN_FILENO, &c, 1) <= 0) {
            tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
            return 0;
        }

        if (c == 27) { // ESC or escape sequence
            // check if more bytes are available
            int bytes = 0;
            ioctl(STDIN_FILENO, FIONREAD, &bytes);
            if (bytes >= 2) {
                unsigned char seq[2];
                read(STDIN_FILENO, seq, 2);
                if (seq[0] == '[') {
                    if (seq[1] == 'A') { tcsetattr(STDIN_FILENO, TCSANOW, &oldt); return 1001; }
                    if (seq[1] == 'B') { tcsetattr(STDIN_FILENO, TCSANOW, &oldt); return 1002; }
                    if (seq[1] == 'C') { tcsetattr(STDIN_FILENO, TCSANOW, &oldt); return 1003; }
                    if (seq[1] == 'D') { tcsetattr(STDIN_FILENO, TCSANOW, &oldt); return 1004; }
                }
            }
            tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
            return 27; // plain ESC
        }

        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        return c;
    };

    while (true) {
        int key = readKey();

        if (key == 27) { // ESC - konec
            break;
        }
        else if (key == 19) { // Ctrl+S - Uložit
            saveToFile(lines);
            // Po uložení vyčisti text a začni nový soubor
            lines = {""};
            curLine = 0;
            curPos = 0;
            renderText(lines, curLine, curPos);
        }
        else if (key == 15) { // Ctrl+O - Otevřít
            if (loadFromFile(lines, curLine, curPos)) {
                renderText(lines, curLine, curPos);
            }
        }
        else if (key == '\n' || key == '\r') { // Enter
            std::string currentLine = lines[curLine];
            std::string newLine = currentLine.substr(curPos);
            lines[curLine] = currentLine.substr(0, curPos);
            lines.insert(lines.begin() + curLine + 1, newLine);
            curLine++;
            curPos = 0;
            renderText(lines, curLine, curPos);
        }
        else if (key == 8 || key == 127) { // Backspace (handle DEL too)
            if (curPos > 0) {
                lines[curLine].erase(curPos - 1, 1);
                curPos--;
            }
            else if (curLine > 0) {
                curPos = (int)lines[curLine - 1].size();
                lines[curLine - 1] += lines[curLine];
                lines.erase(lines.begin() + curLine);
                curLine--;
            }
            renderText(lines, curLine, curPos);
        }
        else if (key >= 1001 && key <= 1004) { // arrows
            if (key == 1001) { // Up
                if (curLine > 0) {
                    curLine--;
                    if (curPos > (int)lines[curLine].size()) curPos = (int)lines[curLine].size();
                }
            }
            else if (key == 1002) { // Down
                if (curLine < (int)lines.size() - 1) {
                    curLine++;
                    if (curPos > (int)lines[curLine].size()) curPos = (int)lines[curLine].size();
                }
            }
            else if (key == 1004) { // Left
                if (curPos > 0) {
                    curPos--;
                }
                else if (curLine > 0) {
                    curLine--;
                    curPos = (int)lines[curLine].size();
                }
            }
            else if (key == 1003) { // Right
                if (curPos < (int)lines[curLine].size()) {
                    curPos++;
                }
                else if (curLine < (int)lines.size() - 1) {
                    curLine++;
                    curPos = 0;
                }
            }
            renderText(lines, curLine, curPos);
        }
        else if (key >= 32 && key <= 126) { // Normální znaky
            lines[curLine].insert(lines[curLine].begin() + curPos, (char)key);
            curPos++;
            renderText(lines, curLine, curPos);
        }
        // jinak ignoruj
    }
}

int main() {
    system("title FastBox Text Editor");

    runEditor();

    return 0;
}
