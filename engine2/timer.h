// engine2/timer.h
// Phase 7 — Timing utilities for the search.
// Portable monotonic clock implementation.

#pragma once

#include <chrono>
#include <atomic>
#include <mutex>

// Returns the current time in milliseconds using a monotonic clock.
inline long long nowMs()
{
    using namespace std::chrono;
    return duration_cast<milliseconds>(
        steady_clock::now().time_since_epoch()
    ).count();
}

// Shared atomic stop flag — thread-safe.
// The search reads this; the caller (or UCI thread) may set it.
inline std::atomic<bool> g_stopSearch{false};
inline std::mutex g_protocolOutputMutex;

// Request the search to stop as soon as possible.
inline void requestStop()
{
    g_stopSearch.store(true, std::memory_order_relaxed);
}

// Clear the stop flag before starting a new search.
inline void clearStop()
{
    g_stopSearch.store(false, std::memory_order_relaxed);
}

// Returns true when the search has been asked to stop.
inline bool isStopRequested()
{
    return g_stopSearch.load(std::memory_order_relaxed);
}
