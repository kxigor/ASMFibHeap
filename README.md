# Fibonacci Heap

![FibHeap](./images/art.png "Fibonacci Heap Graph")

## Requirements
- C compiler (GCC/Clang)
- C++20 (g++/clang++) — for the GoogleTest suite and the demo driver
- NASM — for the assembly heap (optional; a pure-C build works without it)
- Graphviz (`dot`) — for the heap visualizer
- CMake ≥ 3.25 and Ninja
- GoogleTest — for running the test suite
- gcovr — for coverage reports (`pip install --user gcovr`)

## Requirements for the heap only
- C or NASM

## Building

The project uses CMake presets (see `CMakePresets.json`). Configure and build a
preset, then everything lands under `build/<preset>/`:

```bash
cmake --preset dev-debug-asan      # Debug + Address/UB sanitizers + demo apps
cmake --build build/dev-debug-asan
```

Other useful presets:

| Preset              | What it gives you                                  |
|---------------------|----------------------------------------------------|
| `dev-debug-asan`    | Debug, AddressSanitizer + UBSan, apps + tests      |
| `dev-debug-tsan`    | Debug, ThreadSanitizer                             |
| `dev-debug-ubsan`   | Debug, UBSan only                                  |
| `ci-coverage`       | Debug, gcov instrumentation, coverage targets      |
| `ci-relwithdebinfo` | Optimized with debug info                          |
| `ci-release`        | Maximum optimization, no sanitizers                |

To build without the assembly backend (no NASM installed), pass
`-DASMFIBHEAP_BUILD_ASM=OFF`.

## Running the demo

```bash
./build/dev-debug-asan/bin/asm_fib_heap
```

An interactive REPL for the assembly heap. Every inserted key gets a short id
(`#1`, `#2`, ...) so you can refer back to it for `delete` / `decrease`:

| Command               | Alias    | Action                                    |
|-----------------------|----------|-------------------------------------------|
| `insert <key>`        | `i`, `1` | insert a key, prints its id               |
| `extract`             | `e`, `2` | remove and print the minimum              |
| `min`                 | `m`      | show the minimum without removing it      |
| `delete <id>`         | `d`, `3` | delete the node with that id              |
| `decrease <id> <key>` | `c`, `4` | lower an existing node's key              |
| `size`                | `s`      | number of elements in the heap            |
| `draw`                | `g`      | render the heap to `fib_heap.png`         |
| `autodraw <on\|off>`  |          | toggle auto-render after each change       |
| `help`                | `h`, `?` | list the commands                         |
| `quit`                | `q`      | exit                                      |

With auto-draw on (the default), the heap is re-rendered to `fib_heap.png`
after each change. The numeric aliases let you feed batch scripts on stdin.

#### Example
```text
insert 1
insert 2
insert 3
insert 6
insert 5
delete 4
extract
```
Result:
![Result](./images/example.png "I love fibheap")

## Running the tests

The suite is registered with CTest and split into unit, differential
(C vs. assembly), and stress layers:

```bash
cmake --preset dev-debug-asan
cmake --build build/dev-debug-asan
ctest --test-dir build/dev-debug-asan --output-on-failure
```

## Coverage

The core libraries (`Stack`, `FibHeap`, `Graph`) are held at **100% line
coverage**. Build the coverage preset and run one of the gcovr-backed targets:

```bash
cmake --preset ci-coverage
cmake --build build/ci-coverage --target coverage        # HTML + text report
cmake --build build/ci-coverage --target coverage-check  # fails if < 100% lines
```

The HTML report is written to `build/ci-coverage/coverage_report/index.html`.
The remaining uncovered *branches* are exclusively `assert()` and
allocation-failure paths, which cannot be exercised without fault injection.

> Note: this project pins reporting to **gcovr**, not lcov — lcov 2.0
> misparses the `.gcda` format emitted by recent GCC (14+).