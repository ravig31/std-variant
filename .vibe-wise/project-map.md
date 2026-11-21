# Project map

Updated: 21 November 2026.

## Purpose

An educational implementation of a variant to understand templates, recursive storage and object lifetime. The repository records both the implementation and the learning process.

## Files

| File | Role |
| --- | --- |
| `variant.cpp` | Implementation of `ravig31::variant` and the free `ravig31::get` overloads. No `main()` function. |
| `test.cpp` | Runtime assertions, compile-time checks and rejection cases. Includes an AI co-authorship declaration. |
| `Makefile` | Compiles the implementation and runs the dedicated tests. |
| `README.md` | Brief introduction and concepts learnt, based on the learner's notes. |
| `.vibe-wise/progress.md` | Learning milestones, verification and remaining scope. |
| `.vibe-wise/profile.md` | Learning preferences and topics to revisit. |
| `tmp/practice.cpp` | Original local practice implementation. Ignored by Git. |
| `build/` | Generated objects, test executable, dependencies and rejection logs. Ignored by Git. |

The `.vibe-wise` reports are intended to be included in version control. They describe project progress and collaboration, rather than serving as a full chat transcript.

## Implementation

- Namespace `ravig31`, class `variant<AlternativeTypes...>`.
- Recursive `Storage<FirstType, RemainingTypes...>` union with `head` and `tail`, plus a last-type specialisation.
- Exact-type lookup, default and tagged constructors, and an empty-storage tag.
- Copy/move construction, copy/move assignment, assignment from alternative values, destruction and swap.
- Mutable and const member access by index and type, with bounds, uniqueness and active-index checks.
- Free index-based `get` returns references; free type-based `get` returns values to match the requested API.
- Internal empty index `-1`; public `index()` returns `std::size_t`, so empty is exposed as `std::size_t(-1)`. `valueless_by_exception()` queries that state directly.

## Data and lifetime flow

Construction finds the alternative index at compile time and constructs the matching storage path. Assignment reuses an existing alternative when indices match; otherwise it destroys the old value, marks the variant empty and constructs the replacement before publishing its new index.

Destruction traverses only when a value is present. Same-index swap uses ADL-aware value swapping; different-index swap uses a temporary and move operations.

## Build and tests

Run `make` to compile `variant.cpp` as an object file. Run `make test` to build and execute `test.cpp`, including expected compilation failures for ambiguous types, missing types and invalid indices. The Makefile defaults to C++23 and keeps assertions enabled for the tests.

`make test` passed on 21 November 2026. Coverage is recorded in `progress.md`. Earlier GCC/Clang C++26 and sanitizer checks also passed during API adaptation.

## Limits and maintenance

This is not a complete replacement for `std::variant`. Exact-match alternative selection, unconditional move-constructor/swap `noexcept`, and the requested free type-based value returns are retained. Full conversion selection, standard special-member constraints, triviality and broader APIs remain out of scope.

The Makefile still has a leftover `run` target/help text referring to practice variables that are no longer defined. Use the verified `make test` target; cleaning up `run` is a separate maintenance task.
