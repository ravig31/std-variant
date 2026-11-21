# std-variant

An implementation of `std::variant` i did over a couple of weekends out of curiosity. I had used stl implementation before in my code and was curious how it worked, also thought it was good opportunity to understand some basic template metaprogramming.

The storage is a recursive union, with an index to track the active alternative. `variant.cpp` contains `ravig31::variant`; `tmp/practice.cpp` is where I built it step by step and is kept locally, outside version control.

I was also curious to test out [vibe-wise](https://github.com/nykooi1/vibe-wise), a Codex plugin that's prioritises learning and understanding when building with AI. For this project, i used it to go through the concepts step by step, writing the implementation with questions, hints and reviews. The [progress report](.vibe-wise/progress.md) records what I learnt, the tests and what remains, alongside the [project map](.vibe-wise/project-map.md) and [learning profile](.vibe-wise/profile.md). I can say I'm overall quite pleased with how it worked -- never giving me the answer straight away, always attempting to explain the problem in a better way for me to then arrive at the solution by myself.

## personal notes while building (expanded)

### Template syntax and type selection

#### Parameter packs

```cpp
template <typename... AlternativeTypes>
```

This collects all types into one pack. It accepts zero or more types.

```cpp
template <typename FirstType, typename... RemainingTypes>
```

This gives the first type a separate name and collects the remaining types into a pack. It requires at least one type.

#### `if constexpr`: using an `else`

An early return does not stop template instantiation. Code outside the discarded branch still gets instantiated.

```cpp
if constexpr (I == 0) {
    return data.head;
} else {
    return access<I - 1>(data.tail);
}
```

The `else` matters here. When `I == 0`, the compiler drops the recursive branch. Without it, the compiler still tries to instantiate the next step, even though the first branch returns.

#### `requires`

A `requires` clause makes a template available only when a compile-time condition is true.

```cpp
template <int N>
    requires(N > 5)
void example() {}
```

#### `.template` before a member template call

```cpp
tail.template create_from_value<I - 1>(
    std::forward<InputType>(value));
```

When the object's type depends on template parameters, the compiler needs to know that the member is a template. The `template` keyword resolves the ambiguity with `<` being parsed as a less-than operator.

#### `std::in_place_index_t`

`std::in_place_index_t<I>` is a small tag type that carries an index in its type. It lets a constructor receive a compile-time index through an ordinary argument.

```cpp
Storage(std::in_place_index_t<I>, InputType&& value);
```

#### `std::remove_cvref_t`

Removes references and top-level `const` or `volatile` qualifiers from a type, e.g. `const int&` becomes plain `int`.

I use it to compare the input type against the alternatives, without references or qualifiers affecting the match.

### `decltype()`

"declared type" is a keyword introduced in C++11 that inspects and yields the exact data type of an expression at compile time without actually evaluating the expression

e.g.

```cpp

const int& ref = 42;
decltype(ref) x = ref; // x has type: const int&

```

### `decltype(auto)`

introduced in C++14, which uses auto syntax to deduce a type from an initializer while applying decltype's exact reference-preserving rules.

this is what's used for our getters to reference preserve the retrieved types.

### Friend Functions

Is a non-member function that a class explicitly grants access to its `private` and `protected` members.

Rule-of-thumb:

Use `friend` <mark>**only when an operation conceptually belongs to your class’s public interface, cannot be written as a normal member function, and needs direct access to private internals** for correctness or performance

e.g.

* **Asymmetric operators:** Overloading `<<`, `>>`, `+`, `==`, etc., where the left operand is another type (e.g., `std::ostream` or a primitive).
* **Symmetric binary operators:** Overloading `a + b` or `a == b` so implicit conversions work equally on both the left and right sides.
* **Two-class bridges:** A function or tightly coupled helper class (like an iterator for a custom container) that must read private data from two classes at once without exposing public getters.

### Object lifetime and moving

#### Converting constructors

A non-`explicit` constructor can allow an implicit conversion from another type:

```cpp
X(int value);           // Allows X object = 5;
explicit X(int value);  // Requires direct initialisation, e.g. X object(5);
```

My constructor from an alternative value is `explicit`. It creates a new variant; assignment updates one that already exists.

#### `std::construct_at`

Constructs an object in existing storage, rather than allocating new memory.

```cpp
std::construct_at(std::addressof(head), value);
```

It takes a typed pointer and the constructor arguments. It also supports construction during constant evaluation where the language rules allow it. Both `construct_at` and `destroy_at` come from `<memory>`.

#### `std::destroy_at`

Calls the destructor of an object at a specific memory location without deallocating the memory underneath.

```cpp
std::destroy_at(std::addressof(head));
```

It is a clean way to end an object's lifetime while keeping the storage. For the variant, only the active alternative gets destroyed.

#### Why moving needs a non-const source

Moving usually needs to steal resources or modify the source's internal state, so move operations take a non-const source. The source still exists afterwards and still needs to be destroyed.

#### `std::forward`

Used with a forwarding reference such as `InputType&&` in a deduced function template. It preserves the input's value category: an lvalue stays an lvalue, and an rvalue is forwarded as an rvalue.

This lets the stored alternative copy or move as appropriate. The value needs to be forwarded at each recursive call too.

### Finding the right swap

#### Argument-dependent lookup

```cpp
using std::swap;
swap(head, other.head);
```

The unqualified call lets the compiler find a custom `swap` in the type's namespace. `std::swap` is available as a fallback. Calling `std::swap` directly skips that lookup for a custom overload.
