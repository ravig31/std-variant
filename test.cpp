// Co-authored with OpenAI Codex, wrote the tests based on the scenarios covered during
// implementation.

#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

#include "variant.cpp"

namespace {

using StringVariant = ravig31::variant<int, std::string>;
using PointerVariant = ravig31::variant<int, std::unique_ptr<int>>;

static_assert(std::is_same_v<decltype(ravig31::get<0>(std::declval<StringVariant&>())), int&>);
static_assert(std::is_same_v<
              decltype(ravig31::get<1>(std::declval<const StringVariant&>())), const std::string&>);
// Index-based and type-based get preserve references and constness.
static_assert(std::is_same_v<
              decltype(ravig31::get<std::string>(std::declval<StringVariant&>())), std::string&>);
static_assert(
    std::is_same_v<decltype(ravig31::get<int>(std::declval<const StringVariant&>())), const int&>);
static_assert(
    std::is_same_v<
        decltype(ravig31::get<std::string>(std::declval<const StringVariant&>())), const std::string&>);
static_assert(std::is_same_v<decltype(std::declval<StringVariant>().index()), std::size_t>);
static_assert(noexcept(std::declval<const StringVariant&>().index()));
static_assert(std::is_nothrow_move_constructible_v<StringVariant>);
static_assert(noexcept(std::declval<StringVariant&>().swap(std::declval<StringVariant&>())));

constexpr bool constant_evaluation() {
    ravig31::variant<int, double> first;
    if (ravig31::get<0>(first) != 0) return false;
    ravig31::variant<int, double> second(3.5);
    const ravig31::variant<int, double> copied(second);
    if (copied.index() != 1 || ravig31::get<double>(copied) != 3.5) return false;
    ravig31::variant<int, double> moved(std::move(second));
    first = 2.5;
    return ravig31::get<1>(moved) == 3.5 && ravig31::get<1>(first) == 2.5;
}
static_assert(constant_evaluation());

void construction_and_access() {
    StringVariant value;
    assert(value.index() == 0);
    assert(ravig31::get<int>(value) == 0);
    assert(!value.valueless_by_exception());

    StringVariant text(std::string{"hello"});
    ravig31::get<std::string>(text) = "changed";
    assert(ravig31::get<1>(text) == "changed");
    const auto& view = text;
    assert(ravig31::get<1>(view) == "changed");
    assert(ravig31::get<std::string>(view) == "changed");
    assert(ravig31::get<std::string>(view) == "changed");

    bool threw = false;
    try {
        (void)ravig31::get<0>(view);
    } catch (const std::bad_variant_access&) {
        threw = true;
    }
    assert(threw);

    ravig31::variant<std::string> single(std::string{"single"});
    assert(ravig31::get<0>(single) == "single");
    ravig31::variant<int, int> duplicate;
    ravig31::get<0>(duplicate) = 12;
    assert(ravig31::get<0>(duplicate) == 12);
}

void get_returns_references() {
    StringVariant value(std::string{"hello"});
    auto& text = ravig31::get<std::string>(value);
    text += " world";
    assert(ravig31::get<std::string>(value) == "hello world");
    assert(std::addressof(text) == std::addressof(ravig31::get<1>(value)));

    const auto& view = value;
    const auto& constant_text = ravig31::get<std::string>(view);
    assert(std::addressof(constant_text) == std::addressof(text));
    assert(constant_text == "hello world");

    PointerVariant pointer(std::make_unique<int>(42));
    auto& stored = ravig31::get<std::unique_ptr<int>>(pointer);
    assert(*stored == 42);
    assert(std::addressof(stored) == std::addressof(ravig31::get<1>(pointer)));

    bool threw = false;
    try {
        (void)ravig31::get<int>(value);
    } catch (const std::bad_variant_access&) {
        threw = true;
    }
    assert(threw);
}

void copying_and_assignment() {
    StringVariant source(std::string{"hello"});
    StringVariant copied(source);  // Non-const source must select the copy constructor.
    const StringVariant constant(source);
    StringVariant const_copy(constant);
    assert(ravig31::get<1>(const_copy) == "hello");
    ravig31::get<1>(copied) = "changed";
    assert(ravig31::get<1>(source) == "hello");

    StringVariant destination(std::string{"old"});
    destination = source;
    assert(ravig31::get<1>(destination) == "hello");
    StringVariant number(42);
    destination = number;
    assert(destination.index() == 0 && ravig31::get<0>(destination) == 42);
    destination = source;
    assert(destination.index() == 1 && ravig31::get<1>(destination) == "hello");
    auto& alias = destination;
    assert(std::addressof(destination = alias) == std::addressof(destination));
    assert(ravig31::get<1>(destination) == "hello");
}

void moving_and_assignment() {
    PointerVariant source(std::make_unique<int>(42));
    PointerVariant moved(std::move(source));
    assert(moved.index() == 1 && *ravig31::get<1>(moved) == 42);
    assert(source.index() == 1 && ravig31::get<1>(source) == nullptr);

    PointerVariant destination(std::make_unique<int>(7));
    destination = std::move(moved);
    assert(*ravig31::get<1>(destination) == 42 && ravig31::get<1>(moved) == nullptr);
    PointerVariant number(99);
    number = std::move(destination);
    assert(number.index() == 1 && *ravig31::get<1>(number) == 42);
    assert(destination.index() == 1 && ravig31::get<1>(destination) == nullptr);
    PointerVariant integer(12);
    number = std::move(integer);
    assert(number.index() == 0 && ravig31::get<0>(number) == 12);

    auto& alias = number;
    assert(std::addressof(number = std::move(alias)) == std::addressof(number));
    assert(ravig31::get<0>(number) == 12);
}

void converting_assignment() {
    StringVariant value;
    assert(std::addressof(value = 42) == std::addressof(value));
    assert(ravig31::get<0>(value) == 42);
    std::string text = "hello";
    value = text;
    assert(value.index() == 1 && ravig31::get<1>(value) == "hello");
    value = std::string{"world"};
    assert(ravig31::get<1>(value) == "world" && text == "hello");
    value = 7;
    assert(value.index() == 0 && ravig31::get<0>(value) == 7);

    PointerVariant pointer;
    pointer = std::make_unique<int>(42);
    assert(pointer.index() == 1 && *ravig31::get<1>(pointer) == 42);
    auto replacement = std::make_unique<int>(99);
    pointer = std::move(replacement);
    assert(*ravig31::get<1>(pointer) == 99 && replacement == nullptr);
}

struct Tracked {
    inline static int alive = 0;
    inline static int destructions = 0;

    Tracked() { ++alive; }
    Tracked(const Tracked&) { ++alive; }
    Tracked(Tracked&&) noexcept { ++alive; }
    ~Tracked() {
        --alive;
        ++destructions;
    }
};

void destruction() {
    Tracked::destructions = 0;
    {
        ravig31::variant<Tracked, int> first;
        assert(Tracked::alive == 1 && Tracked::destructions == 0);
    }
    assert(Tracked::alive == 0 && Tracked::destructions == 1);
    {
        Tracked source;
        {
            ravig31::variant<int, Tracked> tail(source);
            assert(Tracked::alive == 2);
        }
        assert(Tracked::alive == 1 && Tracked::destructions == 2);
    }
    assert(Tracked::alive == 0 && Tracked::destructions == 3);
}

struct ThrowOnCopy {
    ThrowOnCopy() = default;
    ThrowOnCopy(ThrowOnCopy&&) = default;
    ThrowOnCopy(const ThrowOnCopy&) { throw 42; }
    ThrowOnCopy& operator=(const ThrowOnCopy&) = default;
};

void exceptions_and_empty_state() {
    using TestVariant = ravig31::variant<int, ThrowOnCopy>;
    TestVariant empty(7);
    TestVariant source(ThrowOnCopy{});
    bool threw = false;
    try {
        empty = source;
    } catch (int error) {
        assert(error == 42);
        threw = true;
    }
    assert(threw && empty.valueless_by_exception());
    assert(empty.index() == static_cast<std::size_t>(-1));
    bool bad_access = false;
    try {
        (void)ravig31::get<0>(empty);
    } catch (const std::bad_variant_access&) {
        bad_access = true;
    }
    assert(bad_access);

    TestVariant empty_copy(empty);
    TestVariant empty_move(std::move(empty_copy));
    assert(empty_copy.valueless_by_exception() && empty_move.valueless_by_exception());
    TestVariant live(12);
    live = empty;
    assert(live.valueless_by_exception());
    live = empty_move;
    assert(live.valueless_by_exception());
    TestVariant replacement(99);
    live = replacement;
    assert(ravig31::get<0>(live) == 99);
    live = std::move(empty_move);
    assert(live.valueless_by_exception());
    live = TestVariant(77);
    assert(ravig31::get<0>(live) == 77);

    empty.swap(empty_copy);
    assert(empty.valueless_by_exception() && empty_copy.valueless_by_exception());
    live.swap(empty);
    assert(live.valueless_by_exception() && ravig31::get<0>(empty) == 77);
    live.swap(empty);
    assert(ravig31::get<0>(live) == 77 && empty.valueless_by_exception());

    ThrowOnCopy alternative;
    threw = false;
    try {
        live = alternative;
    } catch (int) {
        threw = true;
    }
    assert(threw && live.valueless_by_exception());
    live = 123;
    assert(ravig31::get<0>(live) == 123);
}

namespace custom {
struct Value {
    int number = 0;
};
inline int swaps = 0;
void swap(Value& first, Value& second) noexcept {
    ++swaps;
    std::swap(first.number, second.number);
}
}  // namespace custom

void swapping() {
    StringVariant first(42), second(std::string{"hello"});
    first.swap(second);
    assert(ravig31::get<1>(first) == "hello" && ravig31::get<0>(second) == 42);
    StringVariant third(std::string{"world"});
    first.swap(third);
    assert(ravig31::get<1>(first) == "world" && ravig31::get<1>(third) == "hello");
    second.swap(second);
    assert(ravig31::get<0>(second) == 42);
    StringVariant number(99);
    second.swap(number);
    assert(ravig31::get<0>(second) == 99 && ravig31::get<0>(number) == 42);

    ravig31::variant<int, custom::Value> a(custom::Value{1}), b(custom::Value{2});
    a.swap(b);
    assert(ravig31::get<1>(a).number == 2 && ravig31::get<1>(b).number == 1);
    assert(custom::swaps == 1);
}

}  // namespace

int main() {
#if defined(TEST_DUPLICATE_TYPE)
    ravig31::variant<int, int> value;
    (void)ravig31::get<int>(value);
#elif defined(TEST_MISSING_TYPE)
    StringVariant value;
    (void)ravig31::get<double>(value);
#elif defined(TEST_INVALID_INDEX)
    StringVariant value;
    (void)ravig31::get<2>(value);
#else
    construction_and_access();
    get_returns_references();
    copying_and_assignment();
    moving_and_assignment();
    converting_assignment();
    destruction();
    exceptions_and_empty_state();
    swapping();
    std::cout << "All variant tests passed.\n";
#endif
}
