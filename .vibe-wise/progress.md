# Learning progress

Updated: 21 November 2026.

This records what I worked through while implementing a variant, what I tested, and what still needs revisiting. It is a learning report, not a claim that this matches the full C++ Standard Library implementation.

## How I learnt

I used VibeWise learning mode with Codex for small questions, hints and reviews while writing the practice implementation. I then asked Codex to adapt it to the required API, preserve my names and comments, and write tests for the scenarios we covered. The test file includes an AI co-authorship declaration.

## Storage and templates

- Built a recursive union with a `head`, a `tail` for the remaining alternatives, and a last-type specialisation with no tail.
- Kept an active index separately from the storage. The index is runtime state; the alternative types and storage paths are determined at compile time.
- Used parameter packs, partial specialisation, `sizeof...`, `if constexpr` and `static_assert`.
- Traced index-based access through `tail` with `I - 1`. Learnt why the recursive branch needs an `else` so it is discarded at the base case.

## Type lookup and construction

- Wrote `type_index` to find an exact type match, adding one for each skipped alternative.
- Used `std::remove_cvref_t` to compare the input's underlying type without top-level qualifiers or references.
- Used `std::in_place_index_t<I>` to select a storage constructor with a compile-time index.
- Used `requires` to select constructors and exclude the current variant type from forwarding overloads, so they do not intercept copying or moving.
- Learnt that the class name inside its template refers to the current specialisation.
- Made type lookup static because it depends on types, not an object or its `this` pointer.

## Object lifetime, copying and moving

- Distinguished assignment into a live object from construction in existing storage.
- Used `std::destroy_at` and `std::addressof` to destroy only the active alternative without deallocating the union's storage.
- Used empty union destructors so the active value is not destroyed twice.
- Added an empty-storage tag for constructing a variant's copy or move destination before creating its selected alternative.
- Implemented the Rule of Five: destructor, copy/move constructors and copy/move assignment.
- Used `std::move` at the selected value and learnt that a named rvalue-reference parameter is still an lvalue expression.
- Preserved forwarding through recursive calls with `std::forward`. The source remains alive after moving and still needs destruction.

## Assignment and exceptions

- For matching indices, assign into the existing alternative. For different indices, destroy the old alternative and construct the new one.
- Represented an empty state with internal index `-1`, set before replacement construction. Only update the index after construction succeeds.
- Guarded destruction and traversal for empty sources and destinations, including both-empty assignment.
- Added `valueless_by_exception()` and tested recovery after a throwing copy.
- Added assignment from alternative values, with compile-time selection and forwarding for both copying and moving.

## Swap and access

- Implemented different-index swap with a temporary and three moves, and matching-index swap directly on the active values.
- Added self-swap and empty-state guards. Used `using std::swap; swap(...)` so argument-dependent lookup can find a custom swap.
- Added mutable and const access by index and type, runtime active-index checks, and compile-time index bounds checks.
- Used a fold expression to require exactly one matching type for type-based access. Learnt how multiple packs in one expansion are expanded together.

## Current result and verification

`variant.cpp` contains `ravig31::variant`, with recursive `Storage` unions and the required free `get` functions. The original practice code remains locally in ignored `tmp/practice.cpp`.

The earlier checks were run step by step during learning. They are now captured in `test.cpp`, rather than relying only on chat reports. On 21 November 2026, Codex reran `make test` successfully with the Makefile's C++23 configuration:

- Default and alternative construction, const access, and expected reference/value return types.
- Independent copies, same-index assignment, alternative changes and self-assignment.
- Move-only values through construction, move assignment and assignment from alternative values.
- Destruction counts at the first and nested storage levels.
- Throwing copies, empty-state copying/moving/assignment, and recovery.
- Same-index, different-index, self, empty-state and custom ADL swaps.
- Constant evaluation, and expected compilation failures for duplicate types, missing types and invalid indices.

Earlier adaptation checks also passed with GCC and Clang in C++26 mode, and GCC address/undefined-behaviour sanitizers. Leak detection was disabled because of an environment restriction.

## What remains

The planned educational API is implemented and the current tests pass. Further work would be refinement rather than another required feature.

- Revisit forwarding, overload resolution, fold expressions and lifetime rules through independent examples.
- Broader conversion selection is not implemented; alternatives are selected by exact type match.
- Full standard conformance, special-member constraints, triviality and conditional exception specifications remain outside the current scope.
- Move construction and swap are unconditionally `noexcept`, as retained for this API. Throwing operations on those paths terminate; those failure paths are not runtime-tested.
