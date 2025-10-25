#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <optional>
#include <algorithm>
#include <locale.h>
#include "../interpreter.h"
#include <windows.h>

static inline std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

static void print_help() {
    std::cout <<
        R"(Prikazy (zacinaji dvojteckou):
  :run            Spusti kod v bufferu pres interpreter
  :print          Vypise buffer s cisly radku
  :clear          Vymaze buffer
  :open <soubor>  Nacte buffer ze souboru
  :save <soubor>  Ulozi buffer do souboru
  :help           Zobrazi napovedu
  :quit           Konec
  Otevrete zdrojovy kod, napr. :open kod.txt, pote napiste :run pro spusteni. zdrojovy kod musi byt ve stejne slozce jako tento program.
  
Pozn.: vse ostatni se bere jako zdrojovy kod a pridava se do bufferu.)" << "\n";
}

// Nacteni souboru do stringu
static std::optional<std::string> load_file(const std::string& path, std::string& err) {
    std::ifstream in(path, std::ios::in | std::ios::binary);
    if (!in) { err = "Nelze otevrit soubor: " + path; return std::nullopt; }
    std::ostringstream ss; ss << in.rdbuf();
    return ss.str();
}

// Ulozeni stringu do souboru
static bool save_file(const std::string& path, const std::string& data, std::string& err) {
    std::ofstream out(path, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!out) { err = "Nelze zapsat soubor: " + path; return false; }
    out << data;
    return true;
}

int main(int argc, char** argv) {

    std::vector<std::string> buffer;
    Interpreter interp; // Vas interpreter

    std::cout << "Mini UI pro Interpreter (C++ REPL)\n";
    std::cout << "Zadej :help pro napovedu.\n\n";

    // Pokud je predan soubor jako argument, rovnou ho nacteme
    if (argc >= 2) {
        std::string err;
        auto data = load_file(argv[1], err);
        if (!data) {
            std::cerr << err << "\n";
        }
        else {
            std::stringstream ss(*data);
            std::string line;
            buffer.clear();
            while (std::getline(ss, line)) buffer.push_back(line);
            std::cout << "Nacteno z \"" << argv[1] << "\" (" << buffer.size() << " radku).\n";
        }
    }

    std::string line;

    for (;;) {
        std::cout << (buffer.empty() ? "> " : std::to_string(buffer.size() + 1) + "> ");
        if (!std::getline(std::cin, line)) break;

        // prikaz?
        if (!line.empty() && line[0] == ':') {
            std::string cmdline = trim(line.substr(1));
            std::string cmd, arg;
            {
                std::istringstream is(cmdline);
                is >> cmd;
                std::getline(is, arg);
                arg = trim(arg);
            }

            if (cmd == "help" || cmd == "h" || cmd == "?") {
                print_help();
                continue;
            }
            if (cmd == "quit" || cmd == "q" || cmd == "exit") {
                break;
            }
            if (cmd == "print" || cmd == "p") {
                if (buffer.empty()) { std::cout << "(buffer je prazdny)\n"; continue; }
                for (size_t i = 0; i < buffer.size(); ++i) {
                    std::cout << std::setw(4) << (i + 1) << " | " << buffer[i] << "\n";
                }
                continue;
            }
            if (cmd == "clear" || cmd == "c") {
                buffer.clear();
                std::cout << "Buffer vycisten.\n";
                continue;
            }
            if (cmd == "open" || cmd == "o") {
                if (arg.empty()) { std::cout << "Pouziti: :open cesta/k/souboru\n"; continue; }
                std::string err;
                auto data = load_file(arg, err);
                if (!data) {
                    std::cerr << err << "\n";
                }
                else {
                    std::stringstream ss(*data);
                    std::string l;
                    buffer.clear();
                    while (std::getline(ss, l)) buffer.push_back(l);
                    std::cout << "Nacteno " << buffer.size() << " radku z \"" << arg << "\".\n";
                }
                continue;
            }
            if (cmd == "save" || cmd == "s") {
                if (arg.empty()) { std::cout << "Pouziti: :save cesta/k/souboru\n"; continue; }
                std::ostringstream ss;
                for (size_t i = 0; i < buffer.size(); ++i) {
                    ss << buffer[i];
                    if (i + 1 < buffer.size()) ss << "\n";
                }
                std::string err;
                if (save_file(arg, ss.str(), err)) {
                    std::cout << "Ulozeno do \"" << arg << "\".\n";
                }
                else {
                    std::cerr << err << "\n";
                }
                continue;
            }
            if (cmd == "run" || cmd == "r") {
                // poskladat buffer do jednoho stringu
                std::ostringstream ss;
                for (size_t i = 0; i < buffer.size(); ++i) {
                    ss << buffer[i] << "\n";
                }
                std::string code = ss.str();

                try {
                    std::string out = interp.run(code);
                    if (!out.empty()) std::cout << out;
                }
                catch (const std::string& e) {
                    std::cerr << "Chyba: " << e << "\n";
                }
                catch (const std::exception& e) {
                    std::cerr << "Vyjimka: " << e.what() << "\n";
                }
                catch (...) {
                    std::cerr << "Neznama vyjimka pri behu interpreteru.\n";
                }
                continue;
            }

            std::cout << "Neznamy prikaz. Zkus :help\n";
            continue;
        }

        // jinak je to zdrojovy kod – pridej do bufferu
        buffer.push_back(line);
    }

    std::cout << "Hotovo.\n";
    return 0;
}
