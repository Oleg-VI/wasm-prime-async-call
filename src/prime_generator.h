#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <vector>

/// Sieve-of-Eratosthenes prime generator.
///
/// Computation runs asynchronously via the browser event loop:
/// emscripten_async_call() yields between chunks so the UI stays responsive.
/// No threads, no exceptions.
class PrimeGenerator {
public:
    using ProgressCallback = std::function<void(int /*percent: 0-100*/)>;
    using CompleteCallback = std::function<void(std::vector<int> /*primes*/)>;

    PrimeGenerator() noexcept = default;
    ~PrimeGenerator();

    // Non-copyable, non-movable — owns live async state.
    PrimeGenerator(const PrimeGenerator&)            = delete;
    PrimeGenerator& operator=(const PrimeGenerator&) = delete;

    /// Starts computing all primes below `limit` asynchronously.
    /// @return false if a computation is already in progress.
    [[nodiscard]] bool StartComputation(int limit,
                                        ProgressCallback on_progress,
                                        CompleteCallback on_complete) noexcept;

    /// Cancels an in-progress computation. Safe to call at any time.
    void Cancel() noexcept;

    [[nodiscard]] bool IsRunning() const noexcept;

private:
    // All mutable state carried through the async chain.
    struct ChunkState {
        std::vector<bool>  sieve;
        std::vector<int>   results;
        int                limit{};
        int                sqrt_limit{};
        int                current_factor{2};
        int                chunk_size{};
        ProgressCallback   on_progress;
        CompleteCallback   on_complete;
        std::atomic<bool>  cancelled{false};
        PrimeGenerator*    owner{nullptr};  // null-ed on Cancel()
    };

    static void ProcessChunk(void* raw_state) noexcept;
    static void CollectResults(ChunkState& s) noexcept;

    ChunkState* state_{nullptr};  // heap-owned; null when idle
};