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
// The required free type-based get returns a value; the member returns a reference.
static_assert(std::is_same_v<
              decltype(ravig31::get<std::string>(std::declval<StringVariant&>())), std::string>);
static_assert(
    std::is_same_v<decltype(ravig31::get<int>(std::declval<const StringVariant&>())), int>);
static_assert(
    std::is_same_v<
        decltype(std::declval<const StringVariant&>().get<std::string>()), const std::string&>);
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
    text.get<std::string>() = "changed";
    assert(ravig31::get<1>(text) == "changed");
    const auto& view = text;
    assert(ravig31::get<1>(view) == "changed");
    assert(ravig31::get<std::string>(view) == "changed");
    assert(view.get<std::string>() == "changed");

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

void copying_and_assignment() {
    StringVariant source(std::string{"hello"});
    StringVariant copied(source);  // Non-const source must select the copy constructor.
    const StringVariant constant(source);
    StringVariant const_copy(constant);
    assert(ravig31::get<1>(const_copy) == "hello");
    copied.get<1>() = "changed";
    assert(source.get<1>() == "hello");

    StringVariant destination(std::string{"old"});
    destination = source;
    assert(destination.get<1>() == "hello");
    StringVariant number(42);
    destination = number;
    assert(destination.index() == 0 && destination.get<0>() == 42);
    destination = source;
    assert(destination.index() == 1 && destination.get<1>() == "hello");
    auto& alias = destination;
    assert(std::addressof(destination = alias) == std::addressof(destination));
    assert(destination.get<1>() == "hello");
}

void moving_and_assignment() {
    PointerVariant source(std::make_unique<int>(42));
    PointerVariant moved(std::move(source));
    assert(moved.index() == 1 && *moved.get<1>() == 42);
    assert(source.index() == 1 && source.get<1>() == nullptr);

    PointerVariant destination(std::make_unique<int>(7));
    destination = std::move(moved);
    assert(*destination.get<1>() == 42 && moved.get<1>() == nullptr);
    PointerVariant number(99);
    number = std::move(destination);
    assert(number.index() == 1 && *number.get<1>() == 42);
    assert(destination.index() == 1 && destination.get<1>() == nullptr);
    PointerVariant integer(12);
    number = std::move(integer);
    assert(number.index() == 0 && number.get<0>() == 12);

    auto& alias = number;
    assert(std::addressof(number = std::move(alias)) == std::addressof(number));
    assert(number.get<0>() == 12);
}

void converting_assignment() {
    StringVariant value;
    assert(std::addressof(value = 42) == std::addressof(value));
    assert(value.get<0>() == 42);
    std::string text = "hello";
    value = text;
    assert(value.index() == 1 && value.get<1>() == "hello");
    value = std::string{"world"};
    assert(value.get<1>() == "world" && text == "hello");
    value = 7;
    assert(value.index() == 0 && value.get<0>() == 7);

    PointerVariant pointer;
    pointer = std::make_unique<int>(42);
    assert(pointer.index() == 1 && *pointer.get<1>() == 42);
    auto replacement = std::make_unique<int>(99);
    pointer = std::move(replacement);
    assert(*pointer.get<1>() == 99 && replacement == nullptr);
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
    assert(live.get<0>() == 99);
    live = std::move(empty_move);
    assert(live.valueless_by_exception());
    live = TestVariant(77);
    assert(live.get<0>() == 77);

    empty.swap(empty_copy);
    assert(empty.valueless_by_exception() && empty_copy.valueless_by_exception());
    live.swap(empty);
    assert(live.valueless_by_exception() && empty.get<0>() == 77);
    live.swap(empty);
    assert(live.get<0>() == 77 && empty.valueless_by_exception());

    ThrowOnCopy alternative;
    threw = false;
    try {
        live = alternative;
    } catch (int) {
        threw = true;
    }
    assert(threw && live.valueless_by_exception());
    live = 123;
    assert(live.get<0>() == 123);
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
    assert(first.get<1>() == "hello" && second.get<0>() == 42);
    StringVariant third(std::string{"world"});
    first.swap(third);
    assert(first.get<1>() == "world" && third.get<1>() == "hello");
    second.swap(second);
    assert(second.get<0>() == 42);
    StringVariant number(99);
    second.swap(number);
    assert(second.get<0>() == 99 && number.get<0>() == 42);

    ravig31::variant<int, custom::Value> a(custom::Value{1}), b(custom::Value{2});
    a.swap(b);
    assert(a.get<1>().number == 2 && b.get<1>().number == 1);
    assert(custom::swaps == 1);
}

}  // namespace

int main() {
#if defined(TEST_DUPLICATE_TYPE)
    ravig31::variant<int, int> value;
    (void)value.get<int>();
#elif defined(TEST_MISSING_TYPE)
    StringVariant value;
    (void)value.get<double>();
#elif defined(TEST_INVALID_INDEX)
    StringVariant value;
    (void)value.get<2>();
#else
    construction_and_access();
    copying_and_assignment();
    moving_and_assignment();
    converting_assignment();
    destruction();
    exceptions_and_empty_state();
    swapping();
    std::cout << "All variant tests passed.\n";
#endif
}
