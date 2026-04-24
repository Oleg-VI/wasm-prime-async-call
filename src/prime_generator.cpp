#include "prime_generator.h"

#include <algorithm>
#include <cmath>
#include <emscripten.h>
#include <new>

PrimeGenerator::~PrimeGenerator() {
    Cancel();
}

bool PrimeGenerator::StartComputation(int limit,
                                       ProgressCallback on_progress,
                                       CompleteCallback on_complete) noexcept {
    if (state_) return false;

    // Trivial case: no primes below 2.
    if (limit < 2) {
        on_progress(100);
        on_complete({});
        return true;
    }

    auto* s = new (std::nothrow) ChunkState{};
    if (!s) return false;

    s->limit          = limit;
    s->sqrt_limit     = static_cast<int>(std::sqrt(static_cast<double>(limit)));
    s->current_factor = 2;
    // Tune chunk size: larger chunks = fewer yields but coarser UI updates.
    s->chunk_size     = std::max(50, s->sqrt_limit / 40);
    s->on_progress    = std::move(on_progress);
    s->on_complete    = std::move(on_complete);
    s->cancelled.store(false, std::memory_order_relaxed);
    s->owner          = this;

    s->sieve.assign(static_cast<std::size_t>(limit + 1), true);
    s->sieve[0] = s->sieve[1] = false;

    state_ = s;
    emscripten_async_call(ProcessChunk, s, 0);
    return true;
}

void PrimeGenerator::Cancel() noexcept {
    if (!state_) return;
    state_->cancelled.store(true, std::memory_order_relaxed);
    state_->owner = nullptr;  // Prevent ProcessChunk from touching this object.
    state_        = nullptr;
}

bool PrimeGenerator::IsRunning() const noexcept {
    return state_ != nullptr;
}

// static
void PrimeGenerator::ProcessChunk(void* raw_state) noexcept {
    auto* s = static_cast<ChunkState*>(raw_state);

    if (s->cancelled.load(std::memory_order_relaxed)) {
        delete s;
        return;
    }

    // Process one chunk of factors for the sieve.
    const int end_factor = std::min(s->current_factor + s->chunk_size,
                                    s->sqrt_limit + 1);

    for (int f = s->current_factor; f < end_factor; ++f) {
        if (!s->sieve[static_cast<std::size_t>(f)]) continue;
        // Start from f² — all smaller multiples were already marked.
        for (long long m = static_cast<long long>(f) * f;
             m <= s->limit;
             m += f) {
            s->sieve[static_cast<std::size_t>(m)] = false;
        }
    }

    s->current_factor = end_factor;

    // Report progress (0–99 while sieving, 100 only on completion).
    const int range    = std::max(1, s->sqrt_limit - 2);
    const int progress = std::min(99,
        static_cast<int>(100LL * (s->current_factor - 2) / range));
    s->on_progress(progress);

    if (s->current_factor <= s->sqrt_limit) {
        // More work — yield to the event loop and continue next tick.
        emscripten_async_call(ProcessChunk, s, 0);
        return;
    }

    // Sieve is complete — gather primes and notify the caller.
    CollectResults(*s);
    s->on_progress(100);
    s->on_complete(std::move(s->results));

    if (s->owner) {
        s->owner->state_ = nullptr;
    }
    delete s;
}

// static
void PrimeGenerator::CollectResults(ChunkState& s) noexcept {
    // Pre-allocate to avoid repeated reallocation.
    const auto total = static_cast<std::size_t>(
        std::count(s.sieve.cbegin(), s.sieve.cend(), true));
    s.results.reserve(total);

    for (int i = 2; i <= s.limit; ++i) {
        if (s.sieve[static_cast<std::size_t>(i)]) {
            s.results.push_back(i);
        }
    }
}