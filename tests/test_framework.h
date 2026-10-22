
#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <utility>

namespace TestFramework {

    struct TestResult {
        std::string name;
        bool passed;
        std::string message;
    };

    inline int passed = 0;
    inline int failed = 0;

    inline std::vector<std::string> failures;
    inline std::vector<TestResult> results;

    // Convert values to strings for readable failure messages.
    template <typename T>
    std::string toString(const T& value) {
        std::ostringstream out;
        out << value;
        return out.str();
    }

    // Preferred form: check(name, condition, message)
    inline void check(
        const std::string& name,
        bool condition,
        const std::string& message = ""
    ) {
        if (condition) {
            ++passed;
            results.push_back({name, true, ""});
            std::cout << "[PASS] " << name << '\n';
        } else {
            ++failed;

            std::string details = name;
            if (!message.empty()) {
                details += " | " + message;
            }

            failures.push_back(details);
            results.push_back({name, false, details});
            std::cout << "[FAIL] " << details << '\n';
        }
    }

    // Backward-compatible form: check(condition, name)
    // Backward-compatible form: check(condition, name, message)
    inline void check(
        bool condition,
        const std::string& name,
        const std::string& message = ""
    ) {
        check(name, condition, message);
    }
    
    template <typename Expected, typename Actual>
    void checkEqual(
        const std::string& name,
        const Expected& expected,
        const Actual& actual,
        const std::string& context = ""
    ) {
        const bool equal = (expected == actual);

        std::string message;

        if (!equal) {
            if (!context.empty()) {
                message = context + " | ";
            }

            message += "expected: " + toString(expected)
                    + ", actual: " + toString(actual);
        }

        check(name, equal, message);
    }

    template <typename BoardType>
    void checkFen(
        const std::string& name,
        const BoardType& board,
        const std::string& expectedFen
    ) {
        checkEqual(
            name,
            expectedFen,
            board.toFEN(),
            "FEN comparison"
        );
    }

    // Print a summary of all test results.
    inline void printSummary() {
        std::cout << "\n========== TEST SUMMARY ==========\n";
        std::cout << "Passed: " << passed << '\n';
        std::cout << "Failed: " << failed << '\n';
        std::cout << "Total:  " << passed + failed << '\n';

        if (!failures.empty()) {
            std::cout << "\nFailure details:\n";

            for (const auto& failure : failures) {
                std::cout << "  - " << failure << '\n';
            }
        }

        std::cout << "==================================\n";
    }

    // Reset counters and results before a new run.
    inline void reset() {
        passed = 0;
        failed = 0;
        failures.clear();
        results.clear();
    }

    using TestFunction = void (*)();

    struct RegisteredTest {
        std::string name;
        TestFunction function;
    };

    inline std::vector<RegisteredTest> registeredTests;

    inline void registerTest(
        const std::string& name,
        TestFunction function
    ) {
        registeredTests.push_back({name, function});
    }

    inline int runAllTests() {
        reset();

        for (const auto& test : registeredTests) {
            std::cout << "\n--- " << test.name << " ---\n";
            test.function();
        }

        printSummary();

        return failed == 0 ? 0 : 1;
    }
}
