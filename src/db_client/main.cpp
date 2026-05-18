#include "db_client_lib/db_connection.h"
#include <replxx.hxx>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <cctype>
#include <unistd.h>

using Replxx   = replxx::Replxx;
using RxColor  = Replxx::Color;


static bool g_color = true;

static inline std::string A(const char* code) { return g_color ? code : ""; }

namespace C {
    constexpr auto RST  = "\033[0m";
    constexpr auto BOLD = "\033[1m";
    constexpr auto DIM  = "\033[2m";
    constexpr auto RED  = "\033[31m";
    constexpr auto GRN  = "\033[32m";
    constexpr auto YLW  = "\033[33m";
    constexpr auto BLU  = "\033[34m";
    constexpr auto MGT  = "\033[35m";
    constexpr auto CYN  = "\033[36m";
    constexpr auto GRY  = "\033[90m";
    constexpr auto BGRN = "\033[92m";
    constexpr auto BCYN = "\033[96m";
    constexpr auto WHT  = "\033[97m";
}


static const std::vector<std::string> KW_DML     = {"SELECT","INSERT","UPDATE","DELETE"};
static const std::vector<std::string> KW_DDL     = {"CREATE","DROP","DATABASE","TABLE","USE"};
static const std::vector<std::string> KW_CLAUSE  = {"FROM","WHERE","SET","INTO","VALUES",
                                                     "ORDER","BY","GROUP","LIMIT","OFFSET",
                                                     "ASC","DESC"};
static const std::vector<std::string> KW_LOGIC   = {"AND","OR","NOT","IN","LIKE"};
static const std::vector<std::string> KW_TXN     = {"BEGIN","COMMIT","ROLLBACK"};
static const std::vector<std::string> KW_TYPES   = {"INT","FLOAT","BOOL","TEXT","VARCHAR"};
static const std::vector<std::string> KW_AGG     = {"COUNT","SUM","MIN","MAX","AVG"};
static const std::vector<std::string> KW_LIT     = {"TRUE","FALSE","NULL"};

static std::vector<std::string> allKeywords() {
    std::vector<std::string> all;
    for (auto& v : {KW_DML, KW_DDL, KW_CLAUSE, KW_LOGIC, KW_TXN, KW_TYPES, KW_AGG, KW_LIT})
        all.insert(all.end(), v.begin(), v.end());
    return all;
}

static std::string toUpper(const std::string& s) {
    std::string r = s;
    for (auto& c : r) c = (char)std::toupper((unsigned char)c);
    return r;
}

static std::string firstKeyword(const std::string& q) {
    size_t s = q.find_first_not_of(" \t\n\r");
    if (s == std::string::npos) return "";
    size_t e = q.find_first_of(" \t\n\r;", s);
    return toUpper(e == std::string::npos ? q.substr(s) : q.substr(s, e - s));
}

template<class V>
static bool inVec(const V& v, const std::string& s) {
    return std::find(v.begin(), v.end(), s) != v.end();
}


static void onHighlight(const std::string& in, Replxx::colors_t& colors) {
    size_t i = 0;
    while (i < in.size()) {
        if (std::isspace((unsigned char)in[i])) { ++i; continue; }

        if (in[i] == '\'') {
            size_t s = i++;
            while (i < in.size() && in[i] != '\'') ++i;
            if (i < in.size()) ++i;
            for (size_t j = s; j < i && j < colors.size(); ++j)
                colors[j] = RxColor::YELLOW;
            continue;
        }

        if (std::isdigit((unsigned char)in[i]) ||
            (in[i] == '-' && i+1 < in.size() && std::isdigit((unsigned char)in[i+1]))) {
            size_t s = i;
            if (in[i] == '-') ++i;
            while (i < in.size() && (std::isdigit((unsigned char)in[i]) || in[i] == '.')) ++i;
            for (size_t j = s; j < i && j < colors.size(); ++j)
                colors[j] = RxColor::CYAN;
            continue;
        }

        if (std::isalpha((unsigned char)in[i]) || in[i] == '_') {
            size_t s = i;
            while (i < in.size() && (std::isalnum((unsigned char)in[i]) || in[i] == '_')) ++i;
            std::string up = toUpper(in.substr(s, i - s));

            RxColor col = RxColor::DEFAULT;
            if      (inVec(KW_DML,    up)) col = RxColor::BRIGHTBLUE;
            else if (inVec(KW_DDL,    up)) col = RxColor::BRIGHTMAGENTA;
            else if (inVec(KW_CLAUSE, up)) col = RxColor::BLUE;
            else if (inVec(KW_LOGIC,  up)) col = RxColor::MAGENTA;
            else if (inVec(KW_TXN,    up)) col = RxColor::BROWN;
            else if (inVec(KW_TYPES,  up)) col = RxColor::GREEN;
            else if (inVec(KW_AGG,    up)) col = RxColor::BRIGHTGREEN;
            else if (inVec(KW_LIT,    up)) col = RxColor::CYAN;

            for (size_t j = s; j < i && j < colors.size(); ++j)
                colors[j] = col;
            continue;
        }

        if (i < colors.size()) {
            char ch = in[i];
            if (ch == '*')                          colors[i] = RxColor::BRIGHTRED;
            else if (ch == ';')                     colors[i] = RxColor::WHITE;
            else if (ch == '(' || ch == ')')        colors[i] = RxColor::WHITE;
            else if (ch == ',')                     colors[i] = RxColor::LIGHTGRAY;
            else if (ch=='=' || ch=='<' ||
                     ch=='>' || ch=='!')             colors[i] = RxColor::MAGENTA;
        }
        ++i;
    }
}


static Replxx::completions_t onComplete(const std::string& in, int& ctxLen) {
    Replxx::completions_t out;
    size_t ws = in.size();
    while (ws > 0 && (std::isalnum((unsigned char)in[ws-1]) || in[ws-1]=='_' || in[ws-1]=='\\'))
        --ws;
    std::string pfx = in.substr(ws);
    ctxLen = (int)pfx.size();
    if (pfx.empty()) return out;

    if (pfx[0] == '\\') {
        for (const char* cmd : {"\\help","\\quit","\\exit","\\history","\\clear","\\status"})
            if (std::string(cmd).substr(0, pfx.size()) == pfx)
                out.emplace_back(cmd);
        return out;
    }

    std::string up = toUpper(pfx);
    bool lower = std::islower((unsigned char)pfx[0]);
    for (auto& kw : allKeywords()) {
        if (kw.size() >= up.size() && kw.substr(0, up.size()) == up) {
            std::string comp = kw;
            if (lower) for (auto& c : comp) c = (char)std::tolower((unsigned char)c);
            out.emplace_back(comp);
        }
    }
    return out;
}


static Replxx::hints_t onHint(const std::string& in, int& ctxLen, RxColor& color) {
    Replxx::hints_t hints;
    size_t ws = in.size();
    while (ws > 0 && (std::isalnum((unsigned char)in[ws-1]) || in[ws-1]=='_')) --ws;
    std::string pfx = in.substr(ws);
    if (pfx.size() < 2) return hints;
    std::string up   = toUpper(pfx);
    ctxLen           = (int)pfx.size();
    color            = RxColor::GRAY;
    bool lower       = std::islower((unsigned char)pfx[0]);
    for (auto& kw : allKeywords()) {
        if (kw.size() > up.size() && kw.substr(0, up.size()) == up) {
            std::string h = kw;
            if (lower) for (auto& c : h) c = (char)std::tolower((unsigned char)c);
            hints.emplace_back(h);
            break;
        }
    }
    return hints;
}


static void printTable(const std::vector<std::string>& cols,
                       const std::vector<std::vector<std::string>>& rows) {
    std::vector<size_t> w(cols.size());
    for (size_t i = 0; i < cols.size(); ++i) w[i] = cols[i].size();
    for (auto& row : rows)
        for (size_t i = 0; i < cols.size() && i < row.size(); ++i)
            w[i] = std::max(w[i], row[i].size());

    // horizontal rule helper
    auto hline = [&](const char* l, const char* m, const char* r) {
        std::cout << A(C::DIM) << l;
        for (size_t i = 0; i < cols.size(); ++i) {
            for (size_t j = 0; j < w[i] + 2; ++j) std::cout << "─";
            std::cout << (i + 1 < cols.size() ? m : r);
        }
        std::cout << A(C::RST) << "\n";
    };

    hline("┌", "┬", "┐");

    std::cout << A(C::DIM) << "│" << A(C::RST);
    for (size_t i = 0; i < cols.size(); ++i) {
        std::cout << " " << A(C::BOLD) << A(C::BCYN)
                  << std::left << std::setw((int)w[i]) << cols[i]
                  << A(C::RST) << " " << A(C::DIM) << "│" << A(C::RST);
    }
    std::cout << "\n";

    hline("├", "┼", "┤");

    for (auto& row : rows) {
        std::cout << A(C::DIM) << "│" << A(C::RST);
        for (size_t i = 0; i < cols.size(); ++i) {
            const std::string& val = (i < row.size()) ? row[i] : "";
            std::cout << " ";
            if (val == "NULL") {
                std::cout << A(C::DIM) << A(C::YLW)
                          << std::left << std::setw((int)w[i]) << val
                          << A(C::RST);
            } else {
                bool isNum = !val.empty();
                for (char c : val)
                    if (!std::isdigit((unsigned char)c) && c != '.' && c != '-')
                        { isNum = false; break; }
                if (isNum) std::cout << A(C::CYN);
                std::cout << std::left << std::setw((int)w[i]) << val;
                if (isNum) std::cout << A(C::RST);
            }
            std::cout << " " << A(C::DIM) << "│" << A(C::RST);
        }
        std::cout << "\n";
    }

    hline("└", "┴", "┘");
}


static void printResult(const std::string& raw, double ms) {
    if (raw.empty()) return;

    // Error
    if (raw.size() >= 7 && raw.substr(0, 7) == "ERROR: ") {
        std::cout << A(C::RED) << A(C::BOLD) << "  ✗  " << A(C::RST)
                  << A(C::RED) << raw.substr(7) << A(C::RST);
        return;
    }

    std::istringstream ss(raw);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(ss, line))
        if (!line.empty()) lines.push_back(line);

    bool isSelect = lines.size() >= 2 &&
                    lines[0].find('\t') != std::string::npos &&
                    lines[1].find('-')  != std::string::npos;

    if (isSelect) {
        std::vector<std::string> cols;
        {
            std::istringstream h(lines[0]);
            std::string col;
            while (std::getline(h, col, '\t'))
                if (!col.empty()) cols.push_back(col);
        }

        // Parse rows
        std::vector<std::vector<std::string>> rows;
        for (size_t i = 2; i < lines.size(); ++i) {
            std::vector<std::string> row;
            std::istringstream rs(lines[i]);
            std::string cell;
            while (std::getline(rs, cell, '\t')) row.push_back(cell);
            while (!row.empty() && row.back().empty()) row.pop_back();
            while (row.size() < cols.size()) row.push_back("");
            rows.push_back(row);
        }

        printTable(cols, rows);

        std::cout << A(C::DIM)
                  << "  " << rows.size() << " row" << (rows.size() != 1 ? "s" : "")
                  << " in set (" << std::fixed << std::setprecision(2) << ms << " ms)"
                  << A(C::RST) << "\n";
        return;
    }

    if (raw.find("Affected rows:") != std::string::npos) {
        std::cout << A(C::BGRN) << A(C::BOLD) << "  ✓  " << A(C::RST)
                  << A(C::BGRN) << raw << A(C::RST)
                  << A(C::DIM) << "  (" << std::fixed << std::setprecision(2) << ms << " ms)"
                  << A(C::RST) << "\n";
        return;
    }

    std::cout << A(C::BGRN) << A(C::BOLD) << "  ✓  " << A(C::RST)
              << A(C::BGRN) << raw << A(C::RST)
              << A(C::DIM) << "  (" << std::fixed << std::setprecision(2) << ms << " ms)"
              << A(C::RST) << "\n";
}


static void printHelp() {
    std::cout << "\n"
              << A(C::BOLD) << A(C::BCYN) << "  MyBase CLI  —  Quick Reference\n" << A(C::RST)
              << A(C::DIM)  << "  ──────────────────────────────────────────────────────────────\n" << A(C::RST)
              << "\n"
              << A(C::BOLD) << "  Shell Commands\n" << A(C::RST);

    struct Row { const char* cmd; const char* desc; };
    for (auto& r : std::vector<Row>{
            {"\\help",    "Show this help"},
            {"\\clear",   "Clear the screen"},
            {"\\history", "Show command history"},
            {"\\status",  "Show connection info"},
            {"\\quit",    "Exit MyBase"},
        })
        std::cout << "    " << A(C::YLW)  << std::left << std::setw(12) << r.cmd << A(C::RST)
                  << A(C::DIM) << r.desc  << A(C::RST) << "\n";

    std::cout << "\n" << A(C::BOLD) << "  SQL Reference\n" << A(C::RST);
    for (auto& r : std::vector<Row>{
            {"CREATE DATABASE <n>",          "Create a new database"},
            {"DROP DATABASE <n>",            "Remove a database"},
            {"CREATE TABLE <n> (cols…)",     "Create a table with typed columns"},
            {"DROP TABLE <n>",               "Remove a table"},
            {"SELECT … FROM … WHERE …",      "Query with optional filter, ORDER BY, LIMIT, OFFSET"},
            {"INSERT INTO … VALUES (…)",     "Insert one or more rows"},
            {"UPDATE … SET … WHERE …",       "Update matching rows"},
            {"DELETE FROM … WHERE …",        "Delete matching rows"},
            {"BEGIN / COMMIT / ROLLBACK",    "Transaction control"},
            {"SELECT COUNT/SUM/AVG(…)",      "Aggregate functions with GROUP BY"},
        })
        std::cout << "    " << A(C::BLU) << std::left << std::setw(32) << r.cmd << A(C::RST)
                  << " " << A(C::DIM) << r.desc << A(C::RST) << "\n";

    std::cout << "\n"
              << A(C::DIM)
              << "  Keyboard shortcuts\n"
              << "    Tab       autocomplete keyword       Ctrl+R    reverse history search\n"
              << "    ↑ / ↓     scroll history             Ctrl+C    cancel current input\n"
              << "    Ctrl+D    quit                       Ctrl+L    clear screen\n"
              << "\n"
              << "  Multi-line: keep typing — a query executes when it ends with ';'\n"
              << A(C::RST) << "\n";
}


static void printBanner(const std::string& host, int port) {
    std::cout << "\n";
    std::cout << A(C::BOLD) << A(C::BCYN);
    std::cout
        << "  ███╗   ███╗██╗   ██╗██████╗  █████╗ ███████╗███████╗\n"
        << "  ████╗ ████║╚██╗ ██╔╝██╔══██╗██╔══██╗██╔════╝██╔════╝\n"
        << "  ██╔████╔██║ ╚████╔╝ ██████╔╝███████║███████╗█████╗  \n"
        << "  ██║╚██╔╝██║  ╚██╔╝  ██╔══██╗██╔══██║╚════██║██╔══╝  \n"
        << "  ██║ ╚═╝ ██║   ██║   ██████╔╝██║  ██║███████║███████╗\n"
        << "  ╚═╝     ╚═╝   ╚═╝   ╚═════╝ ╚═╝  ╚═╝╚══════╝╚══════╝\n";
    std::cout << A(C::RST);
    std::cout << "\n"
              << A(C::DIM) << "  Custom SQL Database Engine  •  v1.0\n" << A(C::RST)
              << A(C::BGRN) << "  ✓  Connected to " << host << ":" << port << A(C::RST) << "\n"
              << A(C::DIM)  << "  Type \\help for help  •  End queries with ;\n" << A(C::RST)
              << "\n";
}


int main(int argc, char** argv) {
    std::string host = "127.0.0.1";
    int port = 9000;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--host" && i+1 < argc) host = argv[++i];
        if (arg == "--port" && i+1 < argc) port = std::stoi(argv[++i]);
    }

    // Цвета отключаем при перенаправлении вывода, чтобы ANSI-коды
    // не засоряли файлы и пайпы.
    g_color = isatty(STDOUT_FILENO);

    TcpDbConnection conn(host, port);

    printBanner(host, port);

    Replxx rx;
    rx.install_window_change_handler();

    const char* home = getenv("HOME");
    // История хранится в домашней директории, а не рядом с бинарником,
    // чтобы пережить пересборку проекта.
    std::string histFile = std::string(home ? home : ".") + "/.mybase_history";
    rx.history_load(histFile);
    rx.set_max_history_size(1000);
    rx.set_max_hint_rows(4);
    rx.set_word_break_characters(" \t\n.,;:!?'\"[]{}()<>=+-*/\\");
    rx.set_double_tab_completion(false);
    rx.set_complete_on_empty(false);
    rx.set_beep_on_ambiguous_completion(false);

    rx.set_completion_callback(onComplete);
    rx.set_highlighter_callback(onHighlight);
    rx.set_hint_callback(onHint);

    std::vector<std::string> history;
    std::string accumulated;
    std::string currentDb;
    bool inTransaction = false;
    std::vector<std::string> txBuffer;

    while (true) {
        std::string prompt;
        if (accumulated.empty()) {
            if (g_color) {
                prompt = "\033[1;92mmybase\033[0m";
                if (!currentDb.empty())
                    prompt += "\033[90m(\033[0m\033[93m" + currentDb + "\033[0m\033[90m)\033[0m";
                if (inTransaction)
                    prompt += "\033[35m[txn]\033[0m";
                prompt += "\033[90m > \033[0m";
            } else {
                prompt = currentDb.empty() ? "mybase" : "mybase(" + currentDb + ")";
                if (inTransaction) prompt += "[txn]";
                prompt += " > ";
            }
        } else {
            if (g_color)
                prompt = "\033[90m    -> \033[0m";
            else
                prompt = "    -> ";
        }

        const char* raw = rx.input(prompt);

        if (!raw) {
            std::cout << "\n" << A(C::DIM) << "\n  Goodbye!\n\n" << A(C::RST);
            break;
        }

        std::string line(raw);

        if (accumulated.empty()) {
            if (line == "\\quit" || line == "\\exit" ||
                line == "quit"   || line == "exit") {
                rx.history_add(line);
                std::cout << A(C::DIM) << "\n  Goodbye!\n\n" << A(C::RST);
                break;
            }
            if (line == "\\help") {
                rx.history_add(line);
                printHelp();
                continue;
            }
            if (line == "\\clear") {
                rx.history_add(line);
                std::cout << "\033[2J\033[H";
                printBanner(host, port);
                continue;
            }
            if (line == "\\history") {
                rx.history_add(line);
                if (history.empty()) {
                    std::cout << A(C::DIM) << "  (no history yet)\n" << A(C::RST);
                } else {
                    std::cout << "\n";
                    for (size_t i = 0; i < history.size(); ++i) {
                        std::cout << A(C::DIM) << "  " << std::setw(4) << (i+1) << "  "
                                  << A(C::RST) << history[i] << "\n";
                    }
                    std::cout << "\n";
                }
                continue;
            }
            if (line == "\\status") {
                rx.history_add(line);
                std::cout << "\n"
                          << A(C::DIM)  << "  Host         " << A(C::RST)
                          << A(C::BGRN) << host << ":" << port << A(C::RST) << "\n"
                          << A(C::DIM)  << "  Database     " << A(C::RST)
                          << (currentDb.empty()
                              ? std::string(A(C::DIM)) + "(none)" + A(C::RST)
                              : std::string(A(C::YLW)) + currentDb + A(C::RST)) << "\n"
                          << A(C::DIM)  << "  Transaction  " << A(C::RST)
                          << (inTransaction
                              ? std::string(A(C::MGT)) + "active (" + std::to_string(txBuffer.size()) + " queued)" + A(C::RST)
                              : std::string(A(C::DIM)) + "none" + A(C::RST)) << "\n"
                          << A(C::DIM)  << "  History      " << history.size() << " queries\n\n"
                          << A(C::RST);
                continue;
            }
            if (line.empty()) continue;
        }

        if (!accumulated.empty()) accumulated += "\n";
        accumulated += line;

        const auto last = accumulated.find_last_not_of(" \t\n\r");
        if (last == std::string::npos || accumulated[last] != ';') continue;

        std::string query = accumulated;
        accumulated.clear();

        rx.history_add(query);
        history.push_back(query);

        // Отслеживаем текущую базу, чтобы отображать её в промпте
        // и передавать на сервер в составе каждого запроса.
        {
            std::string trimQ = query;
            size_t s = trimQ.find_first_not_of(" \t\n\r");
            if (s != std::string::npos) trimQ = trimQ.substr(s);
            std::string upQ = toUpper(trimQ.substr(0, 4));
            if (upQ == "USE " || upQ == "USE\t") {
                size_t nameStart = trimQ.find_first_not_of(" \t", 3);
                size_t nameEnd   = trimQ.find_first_of(" \t;", nameStart);
                if (nameStart != std::string::npos)
                    currentDb = trimQ.substr(nameStart,
                        nameEnd == std::string::npos ? std::string::npos : nameEnd - nameStart);
            }
        }

        std::string kw = firstKeyword(query);

        // BEGIN/COMMIT/ROLLBACK перехватываются на стороне клиента:
        // транзакция буферизуется локально и отправляется одним пакетом при COMMIT,
        // чтобы BEGIN и все DML попали в одно TCP-соединение с одной Session.
        if (kw == "BEGIN") {
            std::cout << "\n";
            if (inTransaction) {
                std::cout << A(C::RED) << A(C::BOLD) << "  ✗  " << A(C::RST)
                          << A(C::RED) << "Already in a transaction\n" << A(C::RST);
            } else {
                inTransaction = true;
                txBuffer.clear();
                std::cout << A(C::BGRN) << A(C::BOLD) << "  ✓  " << A(C::RST)
                          << "Transaction started.\n";
            }
            std::cout << "\n";
            continue;
        }

        if (kw == "ROLLBACK") {
            std::cout << "\n";
            if (!inTransaction) {
                std::cout << A(C::RED) << A(C::BOLD) << "  ✗  " << A(C::RST)
                          << A(C::RED) << "No active transaction\n" << A(C::RST);
            } else {
                size_t n = txBuffer.size();
                inTransaction = false;
                txBuffer.clear();
                std::cout << A(C::YLW) << A(C::BOLD) << "  ↩  " << A(C::RST)
                          << "Transaction rolled back";
                if (n > 0)
                    std::cout << " (" << n << " statement" << (n > 1 ? "s" : "") << " discarded)";
                std::cout << ".\n";
            }
            std::cout << "\n";
            continue;
        }

        if (kw == "COMMIT") {
            std::cout << "\n";
            if (!inTransaction) {
                std::cout << A(C::RED) << A(C::BOLD) << "  ✗  " << A(C::RST)
                          << A(C::RED) << "No active transaction\n" << A(C::RST) << "\n";
                continue;
            }
            std::string batch;
            if (!currentDb.empty()) batch = "USE " + currentDb + ";\n";
            batch += "BEGIN;\n";
            for (const auto& stmt : txBuffer) batch += stmt + "\n";
            batch += "COMMIT;";
            inTransaction = false;
            txBuffer.clear();

            auto t0 = std::chrono::high_resolution_clock::now();
            DbResult res = conn.execute(batch);
            double ms = std::chrono::duration<double, std::milli>(
                            std::chrono::high_resolution_clock::now() - t0).count();
            if (!res.success) {
                std::cout << A(C::RED) << A(C::BOLD) << "  ✗  " << A(C::RST)
                          << A(C::RED) << "Connection error — is the server running?\n" << A(C::RST);
            } else {
                printResult(res.output, ms);
            }
            std::cout << "\n";
            continue;
        }

        if (inTransaction) {
            if (kw == "SELECT") {
                // Preview: отправляем BEGIN + буфер + SELECT без COMMIT.
                // Сервер вернёт результат SELECT, видящего uncommitted-изменения,
                // и автоматически откатит транзакцию после ответа.
                std::string batch;
                if (!currentDb.empty()) batch = "USE " + currentDb + ";\n";
                batch += "BEGIN;\n";
                for (const auto& stmt : txBuffer) batch += stmt + "\n";
                batch += query;

                auto t0 = std::chrono::high_resolution_clock::now();
                DbResult res = conn.execute(batch);
                double ms = std::chrono::duration<double, std::milli>(
                                std::chrono::high_resolution_clock::now() - t0).count();
                std::cout << "\n";
                if (!res.success) {
                    std::cout << A(C::RED) << A(C::BOLD) << "  ✗  " << A(C::RST)
                              << A(C::RED) << "Connection error — is the server running?\n" << A(C::RST);
                } else {
                    printResult(res.output, ms);
                }
                std::cout << "\n";
            } else {
                txBuffer.push_back(query);
                std::cout << "\n" << A(C::DIM) << "  ·  Queued (" << txBuffer.size()
                          << " statement" << (txBuffer.size() > 1 ? "s" : "") << " pending)\n"
                          << A(C::RST) << "\n";
            }
            continue;
        }

        // USE db; отправляется первым выражением: оба попадают в одно TCP-соединение
        // и одну Session, поэтому следующий запрос видит выбранную базу данных.
        std::string sendQuery = query;
        if (!currentDb.empty() && toUpper(query.substr(0, 4)) != "USE ")
            sendQuery = "USE " + currentDb + ";\n" + query;

        auto t0 = std::chrono::high_resolution_clock::now();
        DbResult res = conn.execute(sendQuery);
        double ms = std::chrono::duration<double, std::milli>(
                        std::chrono::high_resolution_clock::now() - t0).count();

        std::cout << "\n";
        if (!res.success) {
            std::cout << A(C::RED) << A(C::BOLD) << "  ✗  " << A(C::RST)
                      << A(C::RED) << "Connection error — is the server running?\n" << A(C::RST);
        } else {
            printResult(res.output, ms);
        }
        std::cout << "\n";
    }

    rx.history_save(histFile);
    return 0;
}