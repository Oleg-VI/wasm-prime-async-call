#include <emscripten/bind.h>
#include <emscripten/val.h>

#include "prime_generator.h"

using namespace emscripten;

/// Thin adapter that translates between emscripten::val JS callbacks
/// and the typed C++ std::function API of PrimeGenerator.
///
/// Exposed class name and method names are PascalCase as required.
class PrimeGeneratorAdapter {
public:
    PrimeGeneratorAdapter() = default;

    bool StartComputation(int limit, val on_progress, val on_complete) {
        return generator_.StartComputation(
            limit,
            [on_progress](int percent) {
                on_progress(percent);
            },
            [on_complete](std::vector<int> primes) {
                on_complete(val::array(primes));
            });
    }

    void Cancel() { generator_.Cancel(); }

    bool IsRunning() const { return generator_.IsRunning(); }

private:
    PrimeGenerator generator_;
};

EMSCRIPTEN_BINDINGS(prime_wasm) {
    class_<PrimeGeneratorAdapter>("PrimeGenerator")
        .constructor<>()
        .function("StartComputation", &PrimeGeneratorAdapter::StartComputation)
        .function("Cancel",           &PrimeGeneratorAdapter::Cancel)
        .function("IsRunning",        &PrimeGeneratorAdapter::IsRunning);
}