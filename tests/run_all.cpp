
#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>

#include "test_framework.h"
#include "test_eval.cpp"
#include "test_search.cpp"

// Existing test-suite functions.
void runFenTests();
void runValidationTests();

void runPerftSuiteFast();
void runPerftSuiteSlow();

void runPerftMirrorFast();
void runPerftMirrorSlow();

void runRulesTestsFast();
void runRulesTestsSlow();
void runRulesTestsGate();

void runEvaluationSymmetryTests();
void runEvaluationMaterialTests();
void runEvaluationPieceSquareTests();
void runEvaluationPawnStructureTests();
void runEvaluationMobilityTests();
void runEvaluationKingSafetyTests();
void runEvaluationActivityTests();

// Phase 7 search test suite functions (defined in test_search.cpp via include).
// Declared extern to satisfy the compiler when the file is included.
// (No additional declarations needed — test_search.cpp is #included above.)
// Results for the high-level correctness checklist.
struct GateChecklist
{
    bool fenAndValidation = false;
    bool perft = false;
    bool mirror = false;
    bool rules = false;
};

static GateChecklist gateChecklist;

// Run FEN and position-validation tests together.
void runGateFenAndValidation()
{
    const int failuresBefore = TestFramework::failed;

    runFenTests();
    runValidationTests();

    gateChecklist.fenAndValidation =
        TestFramework::failed == failuresBefore;
}

// Run both fast and slow perft suites.
void runGatePerft()
{
    const int failuresBefore = TestFramework::failed;

    runPerftSuiteFast();
    runPerftSuiteSlow();

    gateChecklist.perft =
        TestFramework::failed == failuresBefore;
}

// Run both fast and slow mirror suites.
void runGateMirror()
{
    const int failuresBefore = TestFramework::failed;

    runPerftMirrorFast();
    runPerftMirrorSlow();

    gateChecklist.mirror =
        TestFramework::failed == failuresBefore;
}

// Run all rule tests, including the 2,000 random games.
void runGateRules()
{
    const int failuresBefore = TestFramework::failed;

    runRulesTestsGate();

    gateChecklist.rules =
        TestFramework::failed == failuresBefore;
}

// Convert a test name to lowercase for reliable matching.
std::string lowercase(std::string value)
{
    std::transform(
        value.begin(),
        value.end(),
        value.begin(),
        [](unsigned char ch)
        {
            return static_cast<char>(std::tolower(ch));
        }
    );

    return value;
}

// Run the complete correctness gate.
// Returns true only when every required category passes.
bool runCorrectnessGate()
{
    gateChecklist = GateChecklist{};

    std::cout
        << "\n========================================\n"
        << "       KNIGHTMARE CORRECTNESS GATE\n"
        << "========================================\n";

    TestFramework::registerTest(
        "FEN and Position Validation",
        runGateFenAndValidation
    );

    TestFramework::registerTest(
        "Complete Perft Regression",
        runGatePerft
    );

    TestFramework::registerTest(
        "Complete Perft Mirror Regression",
        runGateMirror
    );

    TestFramework::registerTest(
        "Rule Tests and Random Stress",
        runGateRules
    );

    // This runs every registered suite and prints the
    // detailed test summary.
    const int testExitCode = TestFramework::runAllTests();

    // Inspect individual results for make/undo and random games.
    bool makeUndoFound = false;
    bool makeUndoPassed = true;

    bool randomGamesPassed = true;
    int randomGamesChecked = 0;

    for (const auto& result : TestFramework::results)
    {
        const std::string name = lowercase(result.name);

        // Include normal make/undo tests and the checks
        // performed during random playouts.
        if (name.find("make/undo identity") != std::string::npos ||
            name.find("make undo identity") != std::string::npos)
        {
            makeUndoFound = true;

            if (!result.passed)
                makeUndoPassed = false;
        }

        // Each random game produces a FEN-consistency result.
        // Count those results to confirm the intended number
        // of games was actually exercised.
        if (name.find("random playout seed ") == 0 &&
            name.find(": fen consistency") != std::string::npos)
        {
            ++randomGamesChecked;

            if (!result.passed)
                randomGamesPassed = false;
        }

        // Any failed random-playout invariant must fail
        // the random-game checklist item.
        if (name.find("random playout seed ") == 0 &&
            !result.passed)
        {
            randomGamesPassed = false;
        }
    }

    const bool makeUndoOK =
        makeUndoFound && makeUndoPassed;

    const bool randomGamesOK =
        randomGamesPassed && randomGamesChecked >= 2000;

    const bool fenOK =
        gateChecklist.fenAndValidation;

    const bool perftOK =
        gateChecklist.perft;

    const bool mirrorOK =
        gateChecklist.mirror;

    const bool rulesOK =
        gateChecklist.rules;

    std::cout
        << "\n========================================\n"
        << "       CORRECTNESS GATE CHECKLIST\n"
        << "========================================\n";

    auto printItem = [](const std::string& label, bool passed)
    {
        std::cout
            << (passed ? "[PASS] " : "[FAIL] ")
            << label << '\n';
    };

    printItem(
        "Perft: all six positions, required depths",
        perftOK
    );

    printItem(
        "Mirror perft: fast and slow suites",
        mirrorOK
    );

    printItem(
        "Rule-specific test suites",
        rulesOK
    );

    printItem(
        "Make/undo identity checks",
        makeUndoOK
    );

    printItem(
        "FEN round trips and position validation",
        fenOK
    );

    printItem(
        "Random games: at least 2,000 completed test runs (" +
            std::to_string(randomGamesChecked) + " checked)",
        randomGamesOK
    );

    const bool passed =
        testExitCode == 0 &&
        perftOK &&
        mirrorOK &&
        rulesOK &&
        makeUndoOK &&
        fenOK &&
        randomGamesOK;

    std::cout
        << "----------------------------------------\n"
        << (passed
                ? "CORRECTNESS GATE: PASS"
                : "CORRECTNESS GATE: FAIL")
        << "\n========================================\n";

    return passed;
}

int main(int argc, char* argv[])
{
    std::string mode = "fast";

    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];

        if (arg == "--fast" ||
            arg == "--slow" ||
            arg == "--gate")
        {
            mode = arg;
        }
        else
        {
            std::cerr << "Unknown argument: " << arg << '\n';
            std::cerr
                << "Usage: run_all [--fast | --slow | --gate]\n";
            return 2;
        }
    }

    if (mode == "--gate")
        return runCorrectnessGate() ? 0 : 1;

    TestFramework::registerTest(
        "FEN Tests",
        runFenTests
    );

    TestFramework::registerTest(
        "Position Validation Tests",
        runValidationTests
    );

    TestFramework::registerTest(
        "Fast Perft Tests",
        runPerftSuiteFast
    );

    TestFramework::registerTest(
        "Perft Mirror Tests",
        runPerftMirrorFast
    );

    TestFramework::registerTest(
        "Evaluation Symmetry Tests",
        runEvaluationSymmetryTests
    );
    TestFramework::registerTest(
        "Evaluation Material Tests",
        runEvaluationMaterialTests
    );
    TestFramework::registerTest(
        "Evaluation Piece-Square Tests",
        runEvaluationPieceSquareTests
    );
    TestFramework::registerTest(
        "Evaluation Pawn Structure Tests",
        runEvaluationPawnStructureTests
    );
    TestFramework::registerTest(
        "Evaluation Mobility Tests",
        runEvaluationMobilityTests
    );
    TestFramework::registerTest(
        "Evaluation King Safety Tests",
        runEvaluationKingSafetyTests
    );
    TestFramework::registerTest(
        "Evaluation Activity Tests",
        runEvaluationActivityTests
    );

    // Phase 7 search tests
    TestFramework::registerTest(
        "Search Tests (Fast)",
        runSearchTestsFast
    );

    if (mode == "--slow")
    {
        TestFramework::registerTest(
            "Game Rules Tests (50 random playouts)",
            runRulesTestsSlow
        );

        TestFramework::registerTest(
            "Slow Perft Tests",
            runPerftSuiteSlow
        );

        TestFramework::registerTest(
            "Slow Perft Mirror Tests",
            runPerftMirrorSlow
        );

        TestFramework::registerTest(
            "Search Tests (Slow / Performance)",
            runSearchTestsSlow
        );
    }
    else
    {
        TestFramework::registerTest(
            "Game Rules Tests (8 random playouts)",
            runRulesTestsFast
        );
    }

    return TestFramework::runAllTests();
}
