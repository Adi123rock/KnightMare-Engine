#include <algorithm>
#include <string>
#include <vector>

#include <emscripten/bind.h>

#include "../engine/Board.h"
#include "../engine/legal.h"
#include "../engine/move.h"
#include "../engine/validate.h"
#include "../engine2/search.h"
#include "../engine2/timer.h"

namespace {
constexpr const char* START_FEN =
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

std::vector<std::string> legalMoveStrings(Board& board) {
    const std::vector<Move> moves = generateLegalMoves(board);
    std::vector<std::string> result;
    result.reserve(moves.size());
    for (const Move& move : moves) result.push_back(moveToString(move));
    return result;
}
}

class KnightmareWasmEngine {
public:
    KnightmareWasmEngine() {
        board_.loadStartPosition();
        initialized_ = true;
    }

    bool reset() {
        Board candidate;
        if (!candidate.loadFEN(START_FEN, &lastError_)) return false;
        board_ = candidate;
        initialized_ = true;
        lastError_.clear();
        return true;
    }

    bool loadFen(const std::string& fen) {
        Board candidate;
        std::string error;
        if (!candidate.loadFEN(fen, &error)) {
            lastError_ = error;
            return false;
        }
        if (!validatePosition(candidate, &error)) {
            lastError_ = error;
            return false;
        }
        board_ = candidate;
        initialized_ = true;
        lastError_.clear();
        return true;
    }

    std::string fen() const {
        if (!initialized_) return {};
        return board_.toFEN();
    }

    emscripten::val legalMoves() {
        emscripten::val result = emscripten::val::array();
        if (!initialized_) {
            lastError_ = "No position is loaded.";
            return result;
        }
        lastError_.clear();
        Board copy = board_;
        const std::vector<std::string> moves = legalMoveStrings(copy);
        for (std::size_t i = 0; i < moves.size(); ++i) result.set(i, moves[i]);
        return result;
    }

    emscripten::val searchBestMove(int depth) {
        emscripten::val result = emscripten::val::object();
        result.set("bestMove", std::string());
        result.set("hasBestMove", false);
        result.set("score", 0);
        result.set("depth", 0);
        result.set("nodes", 0);
        result.set("completed", false);
        result.set("interrupted", false);
        result.set("error", std::string());

        if (!initialized_) return failSearch(result, "No position is loaded.");
        if (depth < 1 || depth > MAX_PLY)
            return failSearch(result, "Depth must be between 1 and " + std::to_string(MAX_PLY) + ".");
        if (searchInProgress_)
            return failSearch(result, "A search is already in progress.");

        searchInProgress_ = true;
        struct SearchGuard {
            bool& flag;
            ~SearchGuard() { flag = false; }
        } guard{searchInProgress_};

        Board root = board_;
        Board moveCheckBoard = root;
        const std::vector<std::string> legal = legalMoveStrings(moveCheckBoard);

        SearchLimits limits;
        limits.maxDepth = depth;
        limits.infinite = false;
        clearStop();
        const SearchResult searchResult = search(root, limits);

        result.set("score", searchResult.score);
        result.set("depth", searchResult.depth);
        result.set("nodes", static_cast<double>(searchResult.nodes));
        result.set("completed", searchResult.completed);
        result.set("interrupted", searchResult.interrupted);

        if (!searchResult.hasBestMove()) {
            lastError_.clear();
            return result;
        }

        const std::string bestMove = moveToString(searchResult.bestMove);
        if (std::find(legal.begin(), legal.end(), bestMove) == legal.end())
            return failSearch(result, "Search returned a move that is not legal in the root position.");

        result.set("bestMove", bestMove);
        result.set("hasBestMove", true);
        lastError_.clear();
        return result;
    }

    std::string lastError() const { return lastError_; }

private:
    emscripten::val failSearch(emscripten::val result, const std::string& error) {
        lastError_ = error;
        result.set("error", error);
        return result;
    }

    Board board_;
    bool initialized_ = false;
    bool searchInProgress_ = false;
    std::string lastError_;
};

EMSCRIPTEN_BINDINGS(knightmare_wasm) {
    emscripten::class_<KnightmareWasmEngine>("KnightmareEngine")
        .constructor<>()
        .function("reset", &KnightmareWasmEngine::reset)
        .function("loadFen", &KnightmareWasmEngine::loadFen)
        .function("fen", &KnightmareWasmEngine::fen)
        .function("legalMoves", &KnightmareWasmEngine::legalMoves)
        .function("searchBestMove", &KnightmareWasmEngine::searchBestMove)
        .function("lastError", &KnightmareWasmEngine::lastError);
}
