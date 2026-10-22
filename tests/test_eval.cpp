
#include <iostream>
#include <string>
#include <vector>

#include "../engine/Board.h"
#include "../engine/Evaluation.h"
#include "../engine/eval_params.h"
#include "test_framework.h"

// Implemented in test_mirror.cpp.
// Reuse the existing mirror helper; do not redefine it.
std::string mirrorFen(const std::string& fen);

using namespace std;

// Load a FEN and return its side-to-move evaluation.
// Returns a sentinel only if loading fails; the test will report failure.
int evalOf(const string& fen, bool& loaded)
{
    Board board;
    string error;

    loaded = board.loadFEN(fen, &error);

    if (!loaded)
    {
        TestFramework::check(
            "Evaluation test FEN loads",
            false,
            "FEN: " + fen + " | Error: " + error
        );
        return 0;
    }

    return evaluate(board);
}

// Load a FEN and return its White-centric evaluation.
int evalWhiteOf(const string& fen, bool& loaded)
{
    Board board;
    string error;

    loaded = board.loadFEN(fen, &error);

    if (!loaded)
    {
        TestFramework::check(
            "White evaluation test FEN loads",
            false,
            "FEN: " + fen + " | Error: " + error
        );
        return 0;
    }

    return evaluateWhite(board);
}

void runEvaluationSymmetryTests()
{
    cout << "\n========== EVALUATION SYMMETRY TESTS ==========\n";

    // Include varied positions to expose colour-specific evaluation bugs.
    const vector<pair<string, string>> positions =
    {
        {
            "Starting position",
            "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
        },
        {
            "Open centre",
            "r1bqkbnr/pppp1ppp/2n5/4p3/4P3/2N5/PPPP1PPP/R1BQKBNR w KQkq - 2 3"
        },
        {
            "Unequal material",
            "r3k2r/ppp2ppp/2n1bn2/3pp3/3PP3/2N1BN2/PPP2PPP/R3K2R w KQkq - 0 8"
        },
        {
            "Pawn structure",
            "4k3/pp3ppp/8/2pP4/3P4/8/PP3PPP/4K3 w - - 0 1"
        },
        {
            "Passed pawn",
            "4k3/8/4P3/8/8/8/8/4K3 w - - 0 1"
        },
        {
            "Endgame king activity",
            "8/8/4k3/8/4K3/8/4P3/8 w - - 0 1"
        }
    };

    for (const auto& test : positions)
    {
        const string& name = test.first;
        const string& fen = test.second;

        const string mirroredFen = mirrorFen(fen);

        TestFramework::check(
            name + ": mirror FEN is generated",
            !mirroredFen.empty(),
            "Original FEN: " + fen
        );

        if (mirroredFen.empty())
            continue;

        bool originalLoaded = false;
        bool mirroredLoaded = false;

        const int originalEval =
            evalOf(fen, originalLoaded);

        const int mirroredEval =
            evalOf(mirroredFen, mirroredLoaded);

        if (!originalLoaded || !mirroredLoaded)
            continue;

        // Both positions have opposite sides to move, so the
        // side-to-move evaluation should be identical.
        TestFramework::checkEqual(
            name + ": side-to-move mirror symmetry",
            originalEval,
            mirroredEval,
            "Original FEN: " + fen +
            " | Mirrored FEN: " + mirroredFen
        );

        const int originalWhite =
            evalWhiteOf(fen, originalLoaded);

        const int mirroredWhite =
            evalWhiteOf(mirroredFen, mirroredLoaded);

        if (!originalLoaded || !mirroredLoaded)
            continue;

        // The White-centric score must reverse sign.
        TestFramework::checkEqual(
            name + ": White-centric score flips sign",
            originalWhite,
            -mirroredWhite,
            "Original FEN: " + fen +
            " | Mirrored FEN: " + mirroredFen
        );
    }

    // Explicit starting-position tests.
    const string startFen =
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    bool loaded = false;
    const int startWhite = evalWhiteOf(startFen, loaded);

    if (loaded)
    {
        TestFramework::checkEqual(
            "Starting position White-centric evaluation is zero",
            0,
            startWhite
        );

        bool evalLoaded = false;
        const int startEval = evalOf(startFen, evalLoaded);

        if (evalLoaded)
        {
            TestFramework::checkEqual(
                "Starting position evaluation equals tempo bonus",
                EvalParams::TEMPO_BONUS,
                startEval
            );
        }
    }

    // The mirrored starting position has Black to move.
    const string blackToMoveStart =
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1";

    bool blackLoaded = false;
    const int blackStartEval =
        evalOf(blackToMoveStart, blackLoaded);

    if (blackLoaded)
    {
        TestFramework::checkEqual(
            "Black-to-move starting position equals tempo bonus",
            EvalParams::TEMPO_BONUS,
            blackStartEval
        );
    }
}


void runEvaluationMaterialTests()
{
    auto materialOfFEN = [](const std::string& fen, bool& loaded) -> int
    {
        Board board;
        std::string error;
        loaded = board.loadFEN(fen, &error);

        if (!loaded)
        {
            TestFramework::check(
                "Material test FEN loads",
                false,
                "FEN: " + fen + " | Error: " + error
            );
            return 0;
        }

        return evalMaterial(board);
    };

    bool loaded = false;

    // 1. Equal material
    TestFramework::checkEqual(
        "Material: equal material",
        0,
        materialOfFEN(
            "4k3/8/8/8/8/8/8/4K3 w - - 0 1",
            loaded
        )
    );

    // 2. White has one extra pawn
    TestFramework::checkEqual(
        "Material: White extra pawn",
        100,
        materialOfFEN(
            "4k3/8/8/8/8/8/P7/4K3 w - - 0 1",
            loaded
        )
    );

    // 3. Black has one extra pawn
    TestFramework::checkEqual(
        "Material: Black extra pawn",
        -100,
        materialOfFEN(
            "4k3/p7/8/8/8/8/8/4K3 w - - 0 1",
            loaded
        )
    );

    // 4. White has one extra knight
    TestFramework::checkEqual(
        "Material: White extra knight",
        EvalParams::KNIGHT_VALUE,
        materialOfFEN(
            "4k3/8/8/8/8/8/N7/4K3 w - - 0 1",
            loaded
        )
    );

    // 5. White has one extra queen
    TestFramework::checkEqual(
        "Material: White extra queen",
        EvalParams::QUEEN_VALUE,
        materialOfFEN(
            "4k3/8/8/8/8/8/Q7/4K3 w - - 0 1",
            loaded
        )
    );

    // 6. White has a rook; Black has a bishop
    TestFramework::checkEqual(
        "Material: rook versus bishop",
        EvalParams::ROOK_VALUE - EvalParams::BISHOP_VALUE,
        materialOfFEN(
            "4k3/8/8/8/8/8/R7/2b1K3 w - - 0 1",
            loaded
        )
    );

    // 7. Kings alone contribute no material
    TestFramework::checkEqual(
        "Material: kings excluded",
        0,
        materialOfFEN(
            "4k3/8/8/8/8/8/8/4K3 w - - 0 1",
            loaded
        )
    );
}


void runEvaluationPieceSquareTests()
{
    // 1. A white piece and its mirrored black counterpart
    // should contribute equal and opposite scores.
    {
        Board whiteBoard;
        Board blackBoard;
        std::string error;

        bool whiteLoaded = whiteBoard.loadFEN(
            "4k3/8/8/8/3N4/8/8/4K3 w - - 0 1",
            &error
        );

        TestFramework::check(
            "PST: white knight FEN loads",
            whiteLoaded,
            error
        );

        bool blackLoaded = blackBoard.loadFEN(
            "4k3/8/8/8/3n4/8/8/4K3 w - - 0 1",
            &error
        );

        TestFramework::check(
            "PST: black knight FEN loads",
            blackLoaded,
            error
        );

        if (whiteLoaded && blackLoaded)
        {
            const int whiteScore = evalPieceSquare(whiteBoard, 24);
            const int blackScore = evalPieceSquare(blackBoard, 24);

            // The kings are identical in both positions, so they cancel.
            TestFramework::checkEqual(
                "PST: colour symmetry for knight",
                0,
                whiteScore + blackScore
            );
        }
    }

    // 2. White knight placement: central versus rim.
    {
        Board centralBoard;
        Board rimBoard;
        std::string error;

        bool centralLoaded = centralBoard.loadFEN(
            "4k3/8/8/8/3N4/8/8/4K3 w - - 0 1",
            &error
        );

        bool rimLoaded = rimBoard.loadFEN(
            "4k3/8/8/8/8/8/8/N3K3 w - - 0 1",
            &error
        );

        TestFramework::check(
            "PST: central knight FEN loads",
            centralLoaded,
            error
        );

        TestFramework::check(
            "PST: rim knight FEN loads",
            rimLoaded,
            error
        );

        if (centralLoaded && rimLoaded)
        {
            const int centralScore = evalPieceSquare(centralBoard, 24);
            const int rimScore = evalPieceSquare(rimBoard, 24);

            TestFramework::check(
                "PST: central knight scores above rim knight",
                centralScore > rimScore,
                "Central score = " + std::to_string(centralScore) +
                ", rim score = " + std::to_string(rimScore)
            );
        }
    }

    // 3. The king table must blend correctly at both phase extremes.
    {
        const int row = 4;
        const int col = 3;

        const int middleGameValue = pstValue(
            PieceType::k, Colour::white, row, col,
            EvalParams::MAX_PHASE
        );

        const int endGameValue = pstValue(
            PieceType::k, Colour::white, row, col, 0
        );

        TestFramework::checkEqual(
            "PST: king uses middle-game table at maximum phase",
            EvalParams::KING_MG_PST[row][col],
            middleGameValue
        );

        TestFramework::checkEqual(
            "PST: king uses endgame table at zero phase",
            EvalParams::KING_EG_PST[row][col],
            endGameValue
        );
    }

    // 4. Out-of-bounds squares must return zero.
    TestFramework::checkEqual(
        "PST: invalid row returns zero",
        0,
        pstValue(PieceType::n, Colour::white, -1, 3, 24)
    );

    TestFramework::checkEqual(
        "PST: invalid column returns zero",
        0,
        pstValue(PieceType::q, Colour::black, 3, 8, 24)
    );

    // 5. Phase is clamped to the supported range.
    {
        const int phaseBelowRange = pstValue(
            PieceType::k, Colour::white, 4, 3, -10
        );

        const int phaseAtZero = pstValue(
            PieceType::k, Colour::white, 4, 3, 0
        );

        const int phaseAboveRange = pstValue(
            PieceType::k, Colour::white, 4, 3,
            EvalParams::MAX_PHASE + 10
        );

        const int phaseAtMaximum = pstValue(
            PieceType::k, Colour::white, 4, 3,
            EvalParams::MAX_PHASE
        );

        TestFramework::checkEqual(
            "PST: negative phase clamps to zero",
            phaseAtZero,
            phaseBelowRange
        );

        TestFramework::checkEqual(
            "PST: excessive phase clamps to maximum",
            phaseAtMaximum,
            phaseAboveRange
        );
    }
}

void runEvaluationPawnStructureTests()
{
    auto scoreOf = [](const std::string& fen, int phase, bool& loaded) -> int
    {
        Board board;
        std::string error;
        loaded = board.loadFEN(fen, &error);

        if (!loaded)
        {
            TestFramework::check(
                "Pawn structure test FEN loads",
                false,
                "FEN: " + fen + " | Error: " + error
            );
            return 0;
        }

        return evalPawnStructure(board, phase);
    };

    bool loaded = false;

    // No pawns.
    TestFramework::checkEqual(
        "Pawn structure: no pawns",
        0,
        scoreOf("4k3/8/8/8/8/8/8/4K3 w - - 0 1",
                EvalParams::MAX_PHASE, loaded)
    );

    // Isolated white pawn: -18 penalty + 10 passed-pawn bonus.
    TestFramework::checkEqual(
        "Pawn structure: isolated white pawn",
        -EvalParams::ISOLATED_PAWN_PENALTY
            + EvalParams::PASSED_PAWN_BONUS[2],
        scoreOf("4k3/8/8/8/8/8/P7/4K3 w - - 0 1",
                EvalParams::MAX_PHASE, loaded)
    );

    // Isolated black pawn: +18 penalty - 10 passed-pawn bonus.
    TestFramework::checkEqual(
        "Pawn structure: isolated black pawn",
        EvalParams::ISOLATED_PAWN_PENALTY
            - EvalParams::PASSED_PAWN_BONUS[2],
        scoreOf("4k3/p7/8/8/8/8/8/4K3 w - - 0 1",
                EvalParams::MAX_PHASE, loaded)
    );

    // Two white pawns on the same file: one doubled-pawn penalty,
    // two isolated-pawn penalties, and passed-pawn bonuses if applicable.
    TestFramework::checkEqual(
        "Pawn structure: doubled white pawns",
        -EvalParams::DOUBLED_PAWN_PENALTY
            - 2 * EvalParams::ISOLATED_PAWN_PENALTY
            + EvalParams::PASSED_PAWN_BONUS[2]
            + EvalParams::PASSED_PAWN_BONUS[3],
        scoreOf("4k3/8/8/8/8/P7/P7/4K3 w - - 0 1",
                EvalParams::MAX_PHASE, loaded)
    );

    // Adjacent white pawns: neither is isolated; both are passed.
    TestFramework::checkEqual(
        "Pawn structure: adjacent friendly pawns",
        EvalParams::PASSED_PAWN_BONUS[2]
            + EvalParams::PASSED_PAWN_BONUS[2],
        scoreOf("4k3/8/8/8/8/8/PP6/4K3 w - - 0 1",
                EvalParams::MAX_PHASE, loaded)
    );

    // White passed pawn on row 2: relative rank 6.
    TestFramework::checkEqual(
        "Pawn structure: passed white pawn",
        EvalParams::PASSED_PAWN_BONUS[6]
        - EvalParams::ISOLATED_PAWN_PENALTY,
        scoreOf("4k3/8/4P3/8/8/8/8/4K3 w - - 0 1",
                EvalParams::MAX_PHASE, loaded)
    );

    // Black passed pawn on row 1: relative rank 2.
    TestFramework::checkEqual(
    "Pawn structure: passed black pawn",
    EvalParams::ISOLATED_PAWN_PENALTY
        - EvalParams::PASSED_PAWN_BONUS[6],
    scoreOf(
        "4k3/8/8/8/8/p7/8/4K3 w - - 0 1",
        EvalParams::MAX_PHASE,
        loaded
    )
);

    // An opposing pawn directly ahead prevents a pawn from being passed.
    // The two pawns are isolated, so their penalties cancel.
    TestFramework::checkEqual(
        "Pawn structure: opposing pawn blocks passed status",
        0,
        scoreOf("4k3/8/8/4p3/4P3/8/8/4K3 w - - 0 1",
                EvalParams::MAX_PHASE, loaded)
    );

    // Verify endgame scaling for a passed white pawn on row 2.
    {
        Board board;
        std::string error;
        const bool fenLoaded = board.loadFEN(
            "4k3/8/4P3/8/8/8/8/4K3 w - - 0 1",
            &error
        );

        TestFramework::check(
            "Pawn structure: scaling FEN loads",
            fenLoaded,
            error
        );

        if (fenLoaded)
        {
            const int middleGameScore =
                evalPawnStructure(board, EvalParams::MAX_PHASE);

            const int endGameScore =
                evalPawnStructure(board, 0);

            const int baseBonus = EvalParams::PASSED_PAWN_BONUS[6];

            const int expectedEndGameBonus =
                baseBonus *
                EvalParams::PASSED_PAWN_ENDGAME_NUMERATOR /
                EvalParams::PASSED_PAWN_ENDGAME_DENOMINATOR;

            TestFramework::checkEqual(
                "Pawn structure: middle-game bonus",
                baseBonus - EvalParams::ISOLATED_PAWN_PENALTY,
                middleGameScore
            );

            TestFramework::checkEqual(
                "Pawn structure: endgame bonus",
                expectedEndGameBonus - EvalParams::ISOLATED_PAWN_PENALTY,
                endGameScore
            );
        }
    }
}

void runEvaluationMobilityTests()
{
    auto loadBoard = [](Board& board, const std::string& fen) -> bool
    {
        std::string error;
        const bool loaded = board.loadFEN(fen, &error);

        TestFramework::check(
            "Mobility test FEN loads",
            loaded,
            "FEN: " + fen + " | Error: " + error
        );

        return loaded;
    };

    // 1. A knight on d4 has eight available squares on an otherwise
    // empty board, with the kings positioned away from its destinations.
    {
        Board board;
        if (loadBoard(board, "4k3/8/8/8/3N4/8/8/4K3 w - - 0 1"))
        {
            TestFramework::checkEqual(
                "Mobility: central knight has eight moves",
                8,
                countMobility(board, Square{4, 3})
            );
        }
    }

    // 2. A knight in the corner has only two available squares.
    {
        Board board;
        if (loadBoard(board, "4k3/8/8/8/8/8/8/N3K3 w - - 0 1"))
        {
            TestFramework::checkEqual(
                "Mobility: corner knight has two moves",
                2,
                countMobility(board, Square{7, 0})
            );
        }
    }

    // 3. A friendly piece blocks a knight destination.
    {
        Board board;
        if (loadBoard(board, "4k3/8/8/8/3N4/2P5/8/4K3 w - - 0 1"))
        {
            TestFramework::checkEqual(
                "Mobility: friendly piece blocks knight destination",
                8,
                countMobility(board, Square{4, 3})
            );
        }
    }

    // 4. An enemy piece can be captured, so its square counts.
    {
        Board board;
        if (loadBoard(board, "4k3/8/8/8/3N4/2p5/8/4K3 w - - 0 1"))
        {
            TestFramework::checkEqual(
                "Mobility: enemy piece can be captured",
                8,
                countMobility(board, Square{4, 3})
            );
        }
    }

    // 5. A rook on an open board has fourteen available squares.
    {
        Board board;
        if (loadBoard(board, "4k3/8/8/8/3R4/8/8/4K3 w - - 0 1"))
        {
            TestFramework::checkEqual(
                "Mobility: open rook has fourteen moves",
                14,
                countMobility(board, Square{4, 3})
            );
        }
    }

    // 6. A bishop on d4 has thirteen available squares on an open board.
    {
        Board board;
        if (loadBoard(board, "4k3/8/8/8/3B4/8/8/4K3 w - - 0 1"))
        {
            TestFramework::checkEqual(
                "Mobility: open bishop has thirteen moves",
                13,
                countMobility(board, Square{4, 3})
            );
        }
    }

    // 7. Pawns and kings do not receive mobility scores.
    {
        Board board;
        if (loadBoard(board, "4k3/8/8/8/8/8/P7/4K3 w - - 0 1"))
        {
            TestFramework::checkEqual(
                "Mobility: pawn excluded",
                0,
                countMobility(board, Square{6, 0})
            );

            TestFramework::checkEqual(
                "Mobility: king excluded",
                0,
                countMobility(board, Square{7, 4})
            );
        }
    }

    // 8. Baselines and weights match the evaluation parameters.
    TestFramework::checkEqual(
        "Mobility: knight baseline",
        4,
        mobilityBaseline(PieceType::n)
    );

    TestFramework::checkEqual(
        "Mobility: bishop baseline",
        5,
        mobilityBaseline(PieceType::b)
    );

    TestFramework::checkEqual(
        "Mobility: rook baseline",
        7,
        mobilityBaseline(PieceType::r)
    );

    TestFramework::checkEqual(
        "Mobility: queen baseline",
        10,
        mobilityBaseline(PieceType::q)
    );

    TestFramework::checkEqual(
        "Mobility: knight weight",
        4,
        mobilityWeight(PieceType::n)
    );

    TestFramework::checkEqual(
        "Mobility: bishop weight",
        5,
        mobilityWeight(PieceType::b)
    );

    TestFramework::checkEqual(
        "Mobility: rook weight",
        2,
        mobilityWeight(PieceType::r)
    );

    TestFramework::checkEqual(
        "Mobility: queen weight",
        1,
        mobilityWeight(PieceType::q)
    );

    // 9. With no pieces other than kings, mobility score is zero.
    {
        Board board;
        if (loadBoard(board, "4k3/8/8/8/8/8/8/4K3 w - - 0 1"))
        {
            TestFramework::checkEqual(
                "Mobility: kings-only position scores zero",
                0,
                evalMobility(board)
            );
        }
    }

    // 10. White's central knight contributes positively.
    {
        Board board;
        if (loadBoard(board, "4k3/8/8/8/3N4/8/8/4K3 w - - 0 1"))
        {
            const int expected =
                (8 - EvalParams::KNIGHT_MOBILITY_BASELINE)
                * EvalParams::KNIGHT_MOBILITY_WEIGHT;

            TestFramework::checkEqual(
                "Mobility: White knight score",
                expected,
                evalMobility(board)
            );
        }
    }

    // 11. The same knight for Black contributes negatively.
    {
        Board board;
        if (loadBoard(board, "4k3/8/8/8/3n4/8/8/4K3 w - - 0 1"))
        {
            const int expected =
                -(8 - EvalParams::KNIGHT_MOBILITY_BASELINE)
                * EvalParams::KNIGHT_MOBILITY_WEIGHT;

            TestFramework::checkEqual(
                "Mobility: Black knight score",
                expected,
                evalMobility(board)
            );
        }
    }
}

void runEvaluationKingSafetyTests()
{
    auto loadBoard = [](Board& board, const std::string& fen) -> bool
    {
        std::string error;
        const bool loaded = board.loadFEN(fen, &error);

        TestFramework::check(
            "King safety test FEN loads",
            loaded,
            "FEN: " + fen + " | Error: " + error
        );

        return loaded;
    };

    // 1. A complete white pawn shield incurs no shield penalty.
    {
        Board board;
        if (loadBoard(
                board,
                "4k3/8/8/8/8/8/3PPP2/4K3 w - - 0 1"))
        {
            TestFramework::checkEqual(
                "King safety: complete white pawn shield",
                0,
                pawnShieldPenalty(board, Colour::white)
            );
        }
    }

    // 2. A missing pawn in the white king's shield incurs a penalty.
    {
        Board board;
        if (loadBoard(
                board,
                "4k3/8/8/8/8/8/3P1P2/4K3 w - - 0 1"))
        {
            TestFramework::checkEqual(
                "King safety: one missing white shield pawn",
                EvalParams::MISSING_PAWN_SHIELD_PENALTY,
                pawnShieldPenalty(board, Colour::white)
            );
        }
    }

    // 3. The shield is not applied when the white king is away
    // from its home rank.
    {
        Board board;
        if (loadBoard(
                board,
                "4k3/8/8/8/4K3/8/8/8 w - - 0 1"))
        {
            TestFramework::checkEqual(
                "King safety: no shield penalty away from home",
                0,
                pawnShieldPenalty(board, Colour::white)
            );
        }
    }

    // 4. A semi-open file near the white king, with an enemy pawn
    // but no friendly pawn, incurs the semi-open-file penalty.
    {
        Board board;
        if (loadBoard(
                board,
                "4k3/8/8/8/8/8/3PpP2/4K3 w - - 0 1"))
        {
            TestFramework::checkEqual(
                "King safety: semi-open king file",
                EvalParams::SEMI_OPEN_KING_FILE_PENALTY,
                openFilePenalty(board, Colour::white)
            );
        }
    }

    // 5. An open file near the king incurs the open-file penalty.
    {
        Board board;
        if (loadBoard(
                board,
                "4k3/8/8/8/8/8/3P1P2/4K3 w - - 0 1"))
        {
            TestFramework::checkEqual(
                "King safety: open king file",
                EvalParams::OPEN_KING_FILE_PENALTY,
                openFilePenalty(board, Colour::white)
            );
        }
    }

    // 6. An enemy rook on an open king file increases the penalty.
    {
        Board board;
        if (loadBoard(
                board,
                "k3r3/8/8/8/8/8/3P1P2/4K3 w - - 0 1"))
        {
            TestFramework::checkEqual(
                "King safety: enemy rook on open king file",
                EvalParams::OPEN_KING_FILE_HEAVY_PIECE_PENALTY,
                openFilePenalty(board, Colour::white)
            );
        }
    }

    // 7. No enemy attackers in the king zone means zero attack penalty.
    {
        Board board;
        if (loadBoard(
                board,
                "4k3/8/8/8/8/8/8/4K3 w - - 0 1"))
        {
            TestFramework::checkEqual(
                "King safety: no attackers",
                0,
                kingAttackScore(board, Colour::white)
            );
        }
    }

    // 8. King safety is White-centric: a white penalty reduces
    // the final score.
    {
        Board board;
        if (loadBoard(
                board,
                "4k3/8/8/8/8/8/8/4K3 w - - 0 1"))
        {
            const int whitePenalty =
                pawnShieldPenalty(board, Colour::white) +
                openFilePenalty(board, Colour::white) +
                kingAttackScore(board, Colour::white);

            const int blackPenalty =
                pawnShieldPenalty(board, Colour::black) +
                openFilePenalty(board, Colour::black) +
                kingAttackScore(board, Colour::black);

            TestFramework::checkEqual(
                "King safety: White-centric score",
                blackPenalty - whitePenalty,
                evalKingSafety(board, EvalParams::MAX_PHASE)
            );
        }
    }

    // 9. Endgame phase uses 25% of the raw king-safety penalty.
    {
        Board board;
        if (loadBoard(
                board,
                "4k3/8/8/8/8/8/8/4K3 w - - 0 1"))
        {
            const int rawWhitePenalty =
                pawnShieldPenalty(board, Colour::white) +
                openFilePenalty(board, Colour::white) +
                kingAttackScore(board, Colour::white);

            const int rawBlackPenalty =
                pawnShieldPenalty(board, Colour::black) +
                openFilePenalty(board, Colour::black) +
                kingAttackScore(board, Colour::black);

            const int rawScore = rawBlackPenalty - rawWhitePenalty;

            TestFramework::checkEqual(
                "King safety: endgame phase scaling",
                (rawScore * 25) / 100,
                evalKingSafety(board, 0)
            );
        }
    }
}


void runEvaluationActivityTests()
{
    auto loadBoard = [](Board& board, const std::string& fen) -> bool
    {
        std::string error;
        const bool loaded = board.loadFEN(fen, &error);

        TestFramework::check(
            "Activity test FEN loads",
            loaded,
            "FEN: " + fen + " | Error: " + error
        );

        return loaded;
    };

    // --------------------------------------------------
    // 1. Bishop pair: White gets +40.
    // White bishops: c1, f1. Black has only a king.
    // --------------------------------------------------
    {
        Board board;
        if (loadBoard(board, "4k3/8/8/8/8/8/8/2B1KB2 w - - 0 1"))
        {
            TestFramework::checkEqual(
                "Activity: white bishop pair",
                EvalParams::BISHOP_PAIR_BONUS,
                evalBishopPair(board)
            );
        }
    }

    // --------------------------------------------------
    // 2. Bishop pair: Black gets -40.
    // Black bishops: c8, f8. White has only a king.
    // --------------------------------------------------
    {
        Board board;
        if (loadBoard(board, "2b1kb2/8/8/8/8/8/8/4K3 w - - 0 1"))
        {
            TestFramework::checkEqual(
                "Activity: black bishop pair",
                -EvalParams::BISHOP_PAIR_BONUS,
                evalBishopPair(board)
            );
        }
    }

    // --------------------------------------------------
    // 3. One bishop each: neither side has a pair.
    // --------------------------------------------------
    {
        Board board;
        if (loadBoard(board, "4k3/8/8/8/8/8/8/2B1KB2 w - - 0 1"))
        {
            // This position actually has TWO white bishops.
            // It is intentionally not used for the one-bishop test.
        }

        if (loadBoard(board, "4k3/8/8/8/8/8/8/2B1K3 w - - 0 1"))
        {
            TestFramework::checkEqual(
                "Activity: single white bishop gives no pair bonus",
                0,
                evalBishopPair(board)
            );
        }
    }

    // --------------------------------------------------
    // 4. White rook on an open file: +25.
    // The e-file contains no pawns.
    // --------------------------------------------------
    {
        Board board;
        if (loadBoard(board, "4k3/8/8/8/8/8/8/4KR2 w - - 0 1"))
        {
            TestFramework::checkEqual(
                "Activity: white rook on open file",
                25,
                evalRookFiles(board)
            );
        }
    }

    // --------------------------------------------------
    // 5. White rook on a semi-open file: +10.
    // The e-file contains a black pawn but no white pawn.
    // --------------------------------------------------
    {
        // White rook on e1; black pawn on e2.
        // The e-file has an enemy pawn but no white pawns.
        // Expected: semi-open file bonus = +10.
        Board board;
        if (loadBoard(board, "4k3/8/8/8/8/8/4p3/K3R3 w - - 0 1"))
        {
            TestFramework::check(
                "Activity: white rook file is semi-open",
                isSemiOpenFile(board, 4, Colour::white)
            );

            TestFramework::check(
                "Activity: white rook file is not open",
                !isOpenFile(board, 4)
            );

            TestFramework::check(
                "Activity: black pawn is on e2",
                board.getPiece(6, 4) == 'p'
            );

            TestFramework::check(
                "Activity: white rook is on e1",
                board.getPiece(7, 4) == 'R'
            );

            TestFramework::checkEqual(
                "Activity: white rook on semi-open file",
                10,
                evalRookFiles(board)
            );
        }
    }

    // --------------------------------------------------
    // 6. Black rook on an open file: -25.
    // The e-file contains no pawns.
    // --------------------------------------------------
    {
        Board board;
        if (loadBoard(board, "4kr2/8/8/8/8/8/8/4K3 w - - 0 1"))
        {
            TestFramework::checkEqual(
                "Activity: black rook on open file",
                -25,
                evalRookFiles(board)
            );
        }
    }

    // --------------------------------------------------
    // 7. Black rook on a semi-open file: -10.
    // The e-file contains a white pawn but no black pawn.
    // --------------------------------------------------
    {
        // Black rook on e8; white pawn on e2.
        // The e-file has an enemy pawn but no black pawns.
        // Expected: semi-open file bonus = -10.
        Board board;
        if (loadBoard(board, "k3r3/8/8/8/8/8/4P3/4K3 w - - 0 1"))
        {
            TestFramework::check(
                "Activity: black rook file is semi-open",
                isSemiOpenFile(board, 4, Colour::black)
            );

            TestFramework::check(
                "Activity: black rook file is not open",
                !isOpenFile(board, 4)
            );

            TestFramework::check(
                "Activity: white pawn is on e2",
                board.getPiece(6, 4) == 'P'
            );

            TestFramework::check(
                "Activity: black rook is on e8",
                board.getPiece(0, 4) == 'r'
            );

            TestFramework::checkEqual(
                "Activity: black rook on semi-open file",
                -10,
                evalRookFiles(board)
            );
        }
    }
    // --------------------------------------------------
    // 8. White rook on rank 7: +20.
    // Row 1 corresponds to rank 7.
    // --------------------------------------------------
    {
        Board board;
        if (loadBoard(board, "4k3/4R3/8/8/8/8/8/4K3 w - - 0 1"))
        {
            TestFramework::checkEqual(
                "Activity: white rook on seventh rank",
                20,
                evalRookOnSeventh(board)
            );
        }
    }

    // --------------------------------------------------
    // 9. Black rook on rank 2: -20.
    // Row 6 corresponds to rank 2.
    // --------------------------------------------------
    {
        Board board;
        if (loadBoard(board, "4k3/8/8/8/8/8/4r3/4K3 w - - 0 1"))
        {
            TestFramework::checkEqual(
                "Activity: black rook on second rank",
                -20,
                evalRookOnSeventh(board)
            );
        }
    }

    // --------------------------------------------------
    // 10. Combined activity score.
    // White bishop pair: +40.
    // White rook on open file: +25.
    // White rook on rank 7: +20.
    // Total: +85.
    // --------------------------------------------------
    {
        Board board;
        if (loadBoard(board, "4k3/4R3/8/8/8/8/8/2B1KB2 w - - 0 1"))
        {
            TestFramework::checkEqual(
                "Activity: combined white bonuses",
                85,
                evalActivity(board)
            );
        }
    }

    // --------------------------------------------------
    // 11. Combined black activity score.
    // Black bishop pair: -40.
    // Black rook on open file: -25.
    // Black rook on rank 2: -20.
    // Total: -85.
    // --------------------------------------------------
    {
        Board board;
        if (loadBoard(board, "2b1kb2/8/8/8/8/8/4r3/4K3 w - - 0 1"))
        {
            TestFramework::checkEqual(
                "Activity: combined black bonuses",
                -85,
                evalActivity(board)
            );
        }
    }
}


