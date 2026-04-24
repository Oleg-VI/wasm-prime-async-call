# WASM Prime Generator — Cooperative Async (emscripten_async_call)

A WebAssembly prime-number service built in **C++20**, compiled with Emscripten,
and exposed to JavaScript via **embind**. Computation runs without blocking the
browser UI thread using a cooperative chunking strategy driven by
`emscripten_async_call`.

---

## What it does

Finds all prime numbers below a user-supplied limit using the
**Sieve of Eratosthenes**. Progress is reported continuously to the UI during
computation. The page stays fully interactive throughout — the progress bar
updates in real time and the Cancel button is always responsive.

---

## Algorithm

### Sieve of Eratosthenes

1. Allocate a `std::vector<bool>` of size `limit + 1`, initialised to `true`.
2. For every factor `f` from 2 to `√limit`: if `sieve[f]` is still `true`,
   mark all multiples `f², f²+f, f²+2f, …` as composite.
3. Collect all indices still marked `true` — these are the primes.

Time complexity: **O(n log log n)**. Space: **O(n)** bits (one bit per element
thanks to `std::vector<bool>`).

---

## How WASM + Async works here

```
Browser Main Thread (single-threaded event loop)
│
├─ JS loads prime_module.wasm via ES6 import
│
├─ User clicks Start
│   └─ PrimeService.startComputation()
│       └─ C++ PrimeGenerator::StartComputation()
│           └─ emscripten_async_call(ProcessChunk, state, 0)
│                          │
│              ┌───────────┘   (yields to event loop)
│              ▼
│          ProcessChunk() — marks one chunk of multiples
│          on_progress(percent) — JS progress callback
│          emscripten_async_call(ProcessChunk, state, 0)  ← reschedule
│              │
│              └─── ... repeats until √limit is reached
│
├─ on_complete(primes[]) — JS result callback
│
└─ UI events (Cancel, resize, …) handled between every chunk
```

### Key insight

The browser event loop is **cooperative**: a single JS/WASM call that never
returns blocks everything else. `emscripten_async_call` schedules the next
chunk as a **macro-task** (via `setTimeout(fn, 0)` internally), so the browser
gets a chance to process input and repaint between every chunk.

**No threads. No SharedArrayBuffer. No special HTTP headers required.**

---

## Technical decisions

| Decision | Choice | Rationale |
|---|---|---|
| Async model | `emscripten_async_call` + chunked sieve | No pthreads required; works on any static host with zero CORS configuration |
| No exceptions | `new (std::nothrow)`, `[[nodiscard]]`, `bool` return codes | Emscripten can compile without exception support (`-fno-exceptions`); lighter binary |
| `std::vector<bool>` | 1 bit per element | 8× less memory vs `vector<char>`; critical at limits of hundreds of millions |
| Chunk size | `max(50, √limit / 40)` | Empirically tuned: large enough for throughput, small enough for UI responsiveness |
| `ChunkState` on heap | `new ChunkState` | Lifetime spans multiple async callbacks; stack allocation would be unsafe |
| PascalCase exports | `PrimeGenerator`, `StartComputation`, … | Requirement; isolated in `bindings.cpp` adapter so core class is unaffected |
| camelCase JS API | `PrimeService.startComputation()`, … | Requirement; isolated in `prime_service.js` |
| embind over raw WASM imports | `EMSCRIPTEN_BINDINGS` | Type-safe, no manual ABI, supports `std::vector` and `std::function` |

---

## Limitations

| Limitation | Cause |
|---|---|
| UI freezes briefly at start for very large limits (≥ 500 M) | `sieve.assign()` allocates a large contiguous block synchronously before the first chunk |
| UI freezes briefly at end | `val::array(primes)` copies the result vector element-by-element into the JS heap on the main thread |
| Single-core only | Cooperative scheduling on the main thread; no parallelism |
| `int` limit (~2.1 B max) | Intentional; widening to `long long` would require sieve memory that exceeds typical WASM limits anyway |

The start/end freezes are inherent to the single-threaded model and not
fixable without moving to `pthreads` (see the pthread variant).

---

## Pros and cons

**Pros**
- Zero infrastructure requirements — any static file server works.
- No `SharedArrayBuffer`, no COOP/COEP headers needed.
- Simple deployment: two files (`prime_module.js` + `prime_module.wasm`).
- C++ core is straightforward and easy to audit.

**Cons**
- Cannot eliminate UI freezes caused by large allocations or bulk data transfer.
- Chunking logic (chunk size, scheduling) must be manually tuned.
- Progress granularity is limited by chunk size.

---

## Prerequisites

### 1. Git
Download from [git-scm.com](https://git-scm.com). Required to clone emsdk.

### 2. Python 3.x
From [python.org](https://python.org). Required by emsdk and the dev server.
**Add to PATH during installation.**

### 3. CMake 3.20+
```
winget install Kitware.CMake
```

### 4. Ninja
```
winget install Ninja-build.Ninja
```

### 5. Emscripten SDK
```bash
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
emsdk install latest
emsdk activate latest
# Activate the environment (required in every new terminal):
emsdk_env.bat        # cmd
.\emsdk_env.ps1      # PowerShell
```

Visual Studio / MSVC is **not required** — emsdk ships its own Clang/LLD
toolchain.

---

## Build

```batch
# 1. Activate emsdk (every new terminal)
cd C:\emsdk
emsdk_env.bat

# 2. Go to the project and build
cd path\to\prime-wasm
build.bat
```

Output files appear in `web\`:
```
web/
├── prime_module.js    ← Emscripten JS glue
├── prime_module.wasm  ← compiled WebAssembly module
├── index.html
├── prime_service.js
└── server.py
```

---

## Run

```batch
python web\server.py
```

Open **http://localhost:8080** in any modern browser.

---

## Project structure

```
prime-wasm/
├── CMakeLists.txt
├── build.bat
├── src/
│   ├── prime_generator.h      # PrimeGenerator class declaration
│   ├── prime_generator.cpp    # Sieve + async chunking logic
│   └── bindings.cpp           # embind adapter (PascalCase exports)
└── web/
    ├── index.html             # UI
    ├── prime_service.js       # JS wrapper (camelCase API)
    └── server.py              # Dev server with COOP/COEP headers
```
