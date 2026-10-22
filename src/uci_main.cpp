#include "../engine/Board.h"
#include "../engine/legal.h"
#include "../engine/move.h"
#include "../engine2/search.h"
#include "../engine2/timer.h"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace {
Board currentBoard;
std::thread searchThread;
std::atomic<bool> searching{false};
int hashMegabytes = 16;

void sendLine(const std::string& line) {
    std::lock_guard<std::mutex> lock(g_protocolOutputMutex);
    std::cout << line << '\n' << std::flush;
}

void stopAndJoin() {
    if (searchThread.joinable()) {
        requestStop();
        searchThread.join();
    }
    searching.store(false);
}

bool parseInt(const std::string& text, int& value) {
    try {
        std::size_t used = 0;
        long long parsed = std::stoll(text, &used);
        if (used != text.size() || parsed < 0 || parsed > 2147483647LL) return false;
        value = static_cast<int>(parsed);
        return true;
    } catch (...) { return false; }
}

bool parseNodeCount(const std::string& text, std::uint64_t& value) {
    try {
        std::size_t used = 0;
        const unsigned long long parsed = std::stoull(text, &used);
        if (used != text.size() || (!text.empty() && text.front() == '-')) return false;
        value = static_cast<std::uint64_t>(parsed);
        return true;
    } catch (...) { return false; }
}

bool applyUciMove(Board& board, const std::string& text) {
    Move parsed;
    if (!stringToMove(text, parsed)) return false;
    const auto legal = generateLegalMoves(board);
    for (const Move& move : legal) {
        if (moveToString(move) == text) {
            board.makeMove(move);
            return true;
        }
    }
    return false;
}

bool parsePosition(const std::string& line, Board& result) {
    std::istringstream input(line);
    std::string command, kind;
    input >> command >> kind;
    Board candidate;
    if (kind == "startpos") candidate.loadStartPosition();
    else if (kind == "fen") {
        std::vector<std::string> fields;
        std::string token;
        while (input >> token && token != "moves") fields.push_back(token);
        if (fields.size() != 6) return false;
        std::string fen;
        for (std::size_t i = 0; i < fields.size(); ++i) {
            if (i) fen += ' ';
            fen += fields[i];
        }
        if (!candidate.loadFEN(fen)) return false;
        if (token == "moves") {
            while (input >> token) if (!applyUciMove(candidate, token)) return false;
        }
        result = candidate;
        return true;
    } else return false;

    std::string token;
    if (input >> token) {
        if (token != "moves") return false;
        while (input >> token) if (!applyUciMove(candidate, token)) return false;
    }
    result = candidate;
    return true;
}

SearchLimits parseGo(const std::string& line, SideToMove side) {
    SearchLimits limits;
    limits.blackToMove = side == SideToMove::black;
    std::istringstream input(line);
    std::string token;
    input >> token;
    while (input >> token) {
        std::string value;
        if (token == "infinite" || token == "ponder") limits.infinite = true;
        else if (token == "depth" && input >> value) {
            int n; if (parseInt(value, n)) limits.maxDepth = std::clamp(n, 1, MAX_PLY);
        } else if (token == "movetime" && input >> value) {
            parseInt(value, limits.moveTime);
        } else if (token == "wtime" && input >> value) {
            limits.clockSpecified = true;
            parseInt(value, limits.whiteTimeMs);
        } else if (token == "btime" && input >> value) {
            limits.clockSpecified = true;
            parseInt(value, limits.blackTimeMs);
        } else if (token == "winc" && input >> value) {
            parseInt(value, limits.whiteIncMs);
        } else if (token == "binc" && input >> value) {
            parseInt(value, limits.blackIncMs);
        } else if (token == "movestogo" && input >> value) {
            parseInt(value, limits.movesToGo);
        } else if (token == "nodes" && input >> value) {
            parseNodeCount(value, limits.nodeLimit);
        } else if (token == "searchmoves") {
            break; // Not part of the required subset; ignore its remaining list.
        }
    }
    if (limits.moveTime == 0 && limits.whiteTimeMs == 0 && limits.blackTimeMs == 0 &&
        limits.nodeLimit == 0 && limits.maxDepth == MAX_PLY)
        limits.infinite = true;
    return limits;
}

void startSearch(const std::string& line) {
    stopAndJoin();
    Board searchBoard = currentBoard;
    SearchLimits limits = parseGo(line, currentBoard.getSideToMove());
    clearStop();
    searching.store(true);
    searchThread = std::thread([searchBoard = std::move(searchBoard), limits]() mutable {
        SearchResult result = search(searchBoard, limits);
        const std::string move = result.hasBestMove() ? moveToString(result.bestMove) : "0000";
        sendLine("bestmove " + move);
        searching.store(false);
    });
}

void handleSetOption(const std::string& line) {
    std::istringstream input(line);
    std::string token, name, value;
    input >> token >> token; // setoption name
    bool readingValue = false;
    while (input >> token) {
        if (token == "value") { readingValue = true; continue; }
        if (readingValue) { if (!value.empty()) value += ' '; value += token; }
        else { if (!name.empty()) name += ' '; name += token; }
    }
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (lower == "clear hash") globalTranspositionTable().clear();
    else if (lower == "hash") {
        int size = 0;
        if (parseInt(value, size)) {
            hashMegabytes = std::clamp(size, 1, 512);
            globalTranspositionTable().resize(static_cast<std::size_t>(hashMegabytes));
        }
    }
}
}

int main() {
    currentBoard.loadStartPosition();
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream commandInput(line);
        std::string command;
        commandInput >> command;
        if (command == "uci") {
            sendLine("id name Knightmare Engine");
            sendLine("id author Knightmare");
            sendLine("option name Hash type spin default 16 min 1 max 512");
            sendLine("option name Clear Hash type button");
            sendLine("uciok");
        } else if (command == "isready") {
            sendLine("readyok");
        } else if (command == "ucinewgame") {
            stopAndJoin();
            currentBoard.loadStartPosition();
            globalTranspositionTable().clear();
        } else if (command == "position") {
            stopAndJoin();
            Board candidate;
            if (parsePosition(line, candidate)) currentBoard = candidate;
            else std::cerr << "info string invalid position command\n";
        } else if (command == "go") {
            startSearch(line);
        } else if (command == "stop") {
            requestStop();
        } else if (command == "setoption") {
            stopAndJoin();
            handleSetOption(line);
        } else if (command == "debug" || command == "ponderhit") {
            // Accepted no-op commands; no debug stream or ponder state is enabled.
        } else if (command == "quit") {
            stopAndJoin();
            break;
        }
    }
    stopAndJoin();
    return 0;
}
