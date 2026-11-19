// Write your solution here
// C++26 using GCC 16.2
// Debug with std::cerr or std::clog.
// !!! IMPORTANT !!!
// 99% of headers are pre-compiled for you server-side.
// If your submission fails to compile due to a missing header, add it to your
// submission.

#include <cstddef>
namespace dev {

template <typename... Ts> class variant;

// Forward declaration
// so the friend declaration inside variant resolves
template <size_t I, typename... Ts> constexpr decltype(auto) get(const variant<Ts...> &v);

template <typename T, typename... Ts> constexpr T get(const variant<Ts...> &v);

template <typename... Ts> class variant {
  private:
    /* Private data members */
    size_t _active_index;

  public:
    /**
     * @brief: Constructs a variant holding the zero-initialized value
     * of the first alternative.
     */
    constexpr variant() {}

    /**
     * @brief Converting constructor. Constructs a variant holding the
     * alternative of type @p T, from @p x.
     */
    template <typename T> constexpr explicit variant(T &&x) {}

    /**
     * @brief Copy constructor. Constructs a variant holding the same
     * alternative as @p other, copy-constructed from the contained value.
     */
    constexpr variant(const variant &other) {}

    /**
     * @brief Move constructor. Steals data from @p other.
     * Constructs a variant holding the same
     * alternative as @p other, move-constructed from
     * the contained value.
     */
    constexpr variant(variant &&other) noexcept {}

    /**
     * @brief Swaps the contents of two variants.
     *
     * If both variants hold the same alternative (index() == other.index()),
     * the contained values are swapped directly via @c std::swap. Otherwise,
     * the alternatives are exchanged by using a temporary.
     */
    void swap(variant<Ts...> &other) noexcept {}

    /**
     * @brief Copy assignment operator.
     */
    variant<Ts...> &operator=(const variant<Ts...> &other) {}

    /**
     * @brief Move assignment operator.
     */
    variant<Ts...> &operator=(variant<Ts...> &&other) {}

    /**
     * @brief Converting assignment operator. Assigns a value of type
     * @p T to the variant.
     */
    template <typename T> constexpr variant<Ts...> &operator=(T &&element) {}

    /**
     * @brief Destructor. Destroys the currently held alternative.
     */
    constexpr ~variant() {}

    /**
     * @brief Returns the zero-based index of the alternative currently
     * held by the variant.
     */
    constexpr std::size_t index() const noexcept {}

    template <size_t I, typename... Us>
    constexpr friend decltype(auto) get(const variant<Us...> &v);

    template <typename T, typename... Us> constexpr friend T get(const variant<Us...> &v);

    template <size_t I, typename... Us> constexpr friend decltype(auto) get(variant<Us...> &v);

    template <typename T, typename... Us> constexpr friend T get(variant<Us...> &v);
};

// const and non-const versions of get() method
template <size_t I, typename... Ts> constexpr decltype(auto) get(const variant<Ts...> &v) {}

template <typename T, typename... Ts> constexpr T get(const variant<Ts...> &v) {}

template <size_t I, typename... Ts> constexpr decltype(auto) get(variant<Ts...> &v) {}

template <typename T, typename... Ts> constexpr T get(variant<Ts...> &v) {}
} // namespace dev