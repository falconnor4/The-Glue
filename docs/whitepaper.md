# A Canonical ABI for C: A Whitepaper

This document outlines a proposal and proof-of-concept for a canonical Application Binary Interface (ABI) designed to simplify interoperability between C and other programming languages.

## 1. The Problem: C's Unspecified ABI

The C language, despite its ubiquity, lacks a formal, cross-platform ABI specification. As expertly detailed in the article ["C Isn't A Language"](https://faultlore.com/blah/c-isnt-a-language/), what we call "C" is, in practice, a family of platform-and-compiler-specific language implementations.

This leads to significant challenges:
*   **Struct Layout:** The memory layout of `structs` (padding, alignment) is determined by the compiler, leading to incompatibilities when linking libraries built with different compilers or settings.
*   **Calling Conventions:** The rules for how function arguments are passed (e.g., in registers vs. on the stack) and how return values are handled vary significantly between architectures (and even on the same architecture, e.g., Windows vs. Linux x64).
*   **Portability:** Writing truly portable code that interfaces with C requires deep knowledge of multiple platform-specific ABIs and often involves complex, error-prone manual shimming.
*   **Language Interoperability:** For new programming languages, the cost of interfacing with the vast ecosystem of C libraries is immense, as they must implement bespoke support for each target platform's C ABI.

## 2. Proposed Solution: A Canonical ABI Layer

This project does not aim to replace the native C ABI on any given platform. Instead, we propose a practical solution: a well-defined, stable **intermediary ABI** that acts as a lingua franca.

Our proof-of-concept consists of two components:

1.  **A "Canonical ABI" Library (`libcanonical`):** A small C library that provides a concrete implementation of the canonical ABI's rules. It will handle the marshalling (packing) and unmarshalling (unpacking) of data between the native C layout and the canonical layout.

2.  **A Shim Generator Tool (`shim_generator`):** A command-line tool that parses standard C header files. It automatically generates the wrapper code (shims) required to translate function calls and data structures between the native platform ABI and our new canonical ABI.

### Guiding Principles of the Canonical ABI (PoC)

For this proof-of-concept, the Canonical ABI will be defined by the following simple, predictable rules:

*   **Explicit Struct Layout:** All structs will have a standardized, packed layout with no implicit padding. Member alignment will be explicit and consistent across all platforms.
*   **Simplified Calling Convention:** To begin, we will use a simple, stack-based calling convention. All arguments are pushed onto the stack in a defined order. This avoids the immense complexity of platform-specific register allocation rules and provides a stable, universal target.
*   **Metadata-Driven:** The entire translation process is driven by the metadata extracted from the original C header files, ensuring accuracy and automating what is typically a manual, error-prone process.

## 3. Proof-of-Concept Implementation

To validate this approach, we implemented the core components.

### `libcanonical`: The Core Library

The `libcanonical` library was implemented in C. Its core responsibilities are:
1.  **Struct Marshalling:** Provides `marshal_struct` and `unmarshal_struct` functions that convert C structs between their native, platform-specific memory layout and a standardized, packed byte-buffer representation.
2.  **Dynamic Function Calling:** The library's `call_function` uses the popular `libffi` library to dynamically invoke native C function pointers. It uses the `CanonicalFunction` metadata to construct the call interface at runtime, preparing arguments and handling the return value.

### Shims: The Translation Layer

The original goal was to build a `shim_generator` tool that would parse C headers and automatically generate shims. However, writing a C parser robust enough to handle even moderately complex headers proved to be a significant project in its own right and beyond the scope of this PoC.

Instead, we demonstrated the validity of the shim concept by **manually writing the shim file** (`tests/manual_shims.c`) for a simple test library. This file contains:
1.  **Metadata:** A `const CanonicalFunction` struct for each function in the test library, describing its name, return type, and arguments.
2.  **Wrappers:** A `canonical_wrapper_...` function for each original function. This wrapper exposes a simple C-level API that takes a single, packed argument buffer and returns a `CanonicalResult`. Inside, it uses `libcanonical`'s `call_function` to invoke the real, native-ABI function.

## 4. Results: Successful Integration Test

The proof-of-concept was validated with an end-to-end integration test (`tests/run_tests.c`). The test:
1.  Defined a simple C library with three functions: `int add(int, int)`, `void print_message(const char*)`, and `double average(double, double)`.
2.  Manually created the shims and metadata for this library.
3.  Wrote a test runner that prepared canonical argument buffers and called the library functions via their canonical wrappers.
4.  Verified the return values.

The test compiled and ran successfully, proving the core thesis of this project:
> It is possible to create a workable, intermediary ABI by combining runtime metadata, dynamic function call libraries like `libffi`, and a code-generation/shimming strategy.

The output of the successful test run was:
```
=== Running Integration Tests ===
--- Test: add(5, 10) ---
OK: Result was 15
--- Test: print_message("Hello Canonical ABI") ---
Message: Hello Canonical ABI
OK: print_message was called.
--- Test: average(3.0, 7.0) ---
OK: Result was 5.000000
=================================
All 3 tests passed!
```

## 5. Limitations and Future Work

This PoC has several limitations that point the way toward future work:
*   **Automatic Shim Generation:** The manual creation of shims was a shortcut for the PoC. A production-ready version of this system would require a robust C parser, likely built with a dedicated parser-generator tool like `tree-sitter`, `Antlr`, or `Yacc/Bison`.
*   **Complex Types:** The type system handled in this PoC is minimal. A full solution would need to handle more complex cases, most notably passing and returning structs by value, which has highly platform-specific ABI rules.
*   **Performance:** The dynamic nature of `libffi` and the memory copying for marshalling arguments introduces overhead. Future work could investigate JIT-compiling optimized wrappers for specific function signatures to improve performance.
*   **Error Handling:** The error handling is currently very basic. A real library would need a more robust error reporting mechanism.
