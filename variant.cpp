#include <cassert>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>
#include <variant>

namespace ravig31 {

template <typename... AlternativeTypes>
class variant;

// forward declarations (also needed in test.cpp) 
template <std::size_t I, typename... AlternativeTypes>
constexpr decltype(auto) get(const variant<AlternativeTypes...>& value);

template <std::size_t I, typename... AlternativeTypes>
constexpr decltype(auto) get(variant<AlternativeTypes...>& value);

template <typename TargetType, typename... AlternativeTypes>
constexpr decltype(auto) get(const variant<AlternativeTypes...>& value);

template <typename TargetType, typename... AlternativeTypes>
constexpr decltype(auto) get(variant<AlternativeTypes...>& value);

template <typename... AlternativeTypes>
class variant {
    static constexpr int empty_index = -1;

   private:
    struct EmptyStorageTag {};

    template <typename FirstType, typename... RemainingTypes>
    union Storage {
        FirstType head;
        Storage<RemainingTypes...> tail;

        constexpr Storage() : head{} {};

        constexpr Storage(EmptyStorageTag) {};

        template <typename InputType>
        constexpr Storage(std::in_place_index_t<0>, InputType&& value)
            : head{std::forward<InputType>(value)} {}

        template <std::size_t I, typename InputType>
            requires(I > 0)
        constexpr Storage(std::in_place_index_t<I>, InputType&& value)
            : tail{std::in_place_index<I - 1>, std::forward<InputType>(value)} {}

        constexpr ~Storage() {};

        // no template for Storage means its the current specilisation, which makse sense here as we
        // are just copying
        constexpr void create_from_copy(
            const Storage& source, int current_active_index, int travel_count) {
            if (travel_count == current_active_index) {
                std::construct_at(std::addressof(head), source.head);
                return;
            }

            std::construct_at(std::addressof(tail), EmptyStorageTag{});
            tail.create_from_copy(source.tail, current_active_index, travel_count + 1);
        }

        constexpr void assign_from_copy(
            const Storage& source, int current_active_index, int travel_count) {
            if (travel_count == current_active_index) {
                head = source.head;
                return;
            }

            tail.assign_from_copy(source.tail, current_active_index, travel_count + 1);
        }

        constexpr void create_from_move(
            Storage& source, int current_active_index, int travel_count) {
            if (travel_count == current_active_index) {
                std::construct_at(std::addressof(head), std::move(source.head));
                return;
            }

            std::construct_at(std::addressof(tail), EmptyStorageTag{});
            tail.create_from_move(source.tail, current_active_index, travel_count + 1);
        }

        constexpr void assign_from_move(
            Storage& source, int current_active_index, int travel_count) {
            if (travel_count == current_active_index) {
                head = std::move(source.head);
                return;
            }

            tail.assign_from_move(source.tail, current_active_index, travel_count + 1);
        }

        template <std::size_t I, typename InputType>
        constexpr void create_from_value(InputType&& value) {
            if constexpr (I == 0) {
                std::construct_at(std::addressof(head), std::forward<InputType>(value));
            } else {
                std::construct_at(std::addressof(tail), EmptyStorageTag{});
                tail.template create_from_value<I - 1>(std::forward<InputType>(value));
            }
        }

        constexpr void swap_active(Storage& other, int current_active_index, int travel_count) {
            if (current_active_index == travel_count) {
                using std::swap;
                swap(head, other.head);
                return;
            }

            tail.swap_active(other.tail, current_active_index, travel_count + 1);
        }

        constexpr void destroy(int current_active_index, int travel_count) {
            if (travel_count == current_active_index) {
                std::destroy_at(std::addressof(head));
                return;
            }

            tail.destroy(current_active_index, travel_count + 1);
        }
    };

    template <typename LastType>
    union Storage<LastType> {
        LastType head;

        constexpr Storage() : head{} {};

        constexpr Storage(EmptyStorageTag) {};

        template <typename InputType>
        constexpr Storage(std::in_place_index_t<0>, InputType&& value)
            : head{std::forward<InputType>(value)} {}

        constexpr ~Storage() {};

        constexpr void create_from_copy(
            const Storage& source, int current_active_index, int travel_count) {
            assert(travel_count == current_active_index);
            std::construct_at(std::addressof(head), source.head);
            return;
        }

        constexpr void assign_from_copy(
            const Storage& source, int current_active_index, int travel_count) {
            assert(travel_count == current_active_index);
            head = source.head;
        }

        constexpr void create_from_move(
            Storage& source, int current_active_index, int travel_count) {
            assert(travel_count == current_active_index);
            std::construct_at(std::addressof(head), std::move(source.head));
            return;
        }

        constexpr void assign_from_move(
            Storage& source, int current_active_index, int travel_count) {
            assert(travel_count == current_active_index);
            head = std::move(source.head);
        }

        template <std::size_t I, typename InputType>
        constexpr void create_from_value(InputType&& value) {
            static_assert(I == 0, "alternative index out of bounds");
            std::construct_at(std::addressof(head), std::forward<InputType>(value));
        }

        constexpr void swap_active(Storage& other, int current_active_index, int travel_count) {
            assert(current_active_index == travel_count);
            using std::swap;
            swap(head, other.head);
        }

        constexpr void destroy(int current_active_index, int travel_count) {
            assert(travel_count == current_active_index);
            std::destroy_at(std::addressof(head));
        }
    };

    template <typename TargetType, typename CandidateType, typename... RemainingTypes>
    static constexpr std::size_t type_index() {
        if constexpr (std::is_same_v<TargetType, CandidateType>) {
            return 0;
        } else {
            if constexpr (sizeof...(RemainingTypes) > 0) {
                return type_index<TargetType, RemainingTypes...>() + 1;
            } else {
                static_assert(false, "type not recognised");
            }
        }
    }

    template <std::size_t I, typename StorageType>
    constexpr auto& access(StorageType& data) const {
        if constexpr (I == 0)
            return data.head;
        else
            return access<I - 1>(data.tail);
    }

    int _active_index;

    Storage<AlternativeTypes...> _data;

   public:
    constexpr variant() : _active_index{}, _data{} {}

    // copy constructor
    constexpr variant(const variant& other)
        : _active_index{other._active_index}, _data{EmptyStorageTag{}} {
        if (_active_index != empty_index) _data.create_from_copy(other._data, _active_index, 0);
    }

    // copy assignment
    variant& operator=(const variant& other) {
        if (this == std::addressof(other)) return *this;

        // case 1: slot in the source data to the destination
        if (_active_index == other._active_index) {
            if (_active_index != empty_index) _data.assign_from_copy(other._data, _active_index, 0);
        } else {  // case 2: destroy and construct
            if (_active_index != empty_index) {
                _data.destroy(_active_index, 0);
                _active_index = empty_index;
            }

            if (other._active_index != empty_index) {
                _data.create_from_copy(other._data, other._active_index, 0);
                _active_index = other._active_index;
            }
        }

        return *this;
    }

    // move constructor
    constexpr variant(variant&& other) noexcept
        : _active_index{other._active_index}, _data{EmptyStorageTag{}} {
        if (_active_index != empty_index) _data.create_from_move(other._data, _active_index, 0);
    }

    // move assignment
    variant& operator=(variant&& other) {
        if (this == std::addressof(other)) return *this;

        // case 1: slot in the source data to the destination
        if (_active_index == other._active_index) {
            if (_active_index != empty_index) _data.assign_from_move(other._data, _active_index, 0);
        } else {  // case 2: destroy and assign
            if (_active_index != empty_index) {
                _data.destroy(_active_index, 0);
                _active_index = empty_index;
            }

            if (other._active_index != empty_index) {
                _data.create_from_move(other._data, other._active_index, 0);
                _active_index = other._active_index;
            }
        }

        return *this;
    }

    // explicit constructor from an AlternativeType
    template <typename InputType>
        requires(!std::is_same_v<std::remove_cvref_t<InputType>, variant>)
    constexpr explicit variant(InputType&& value)
        : _active_index(type_index<std::remove_cvref_t<InputType>, AlternativeTypes...>()),
          _data(
              std::in_place_index_t<
                  type_index<std::remove_cvref_t<InputType>, AlternativeTypes...>()>(),
              std::forward<InputType>(value)) {}

    // converting assignment
    template <typename InputType>
        requires(!std::is_same_v<std::remove_cvref_t<InputType>, variant>)
    constexpr variant<AlternativeTypes...>& operator=(InputType&& value) {
        constexpr auto target_index =
            type_index<std::remove_cvref_t<InputType>, AlternativeTypes...>();

        // active type is already the input type, just slot in the new value
        if (_active_index == target_index) {
            ravig31::get<target_index>(*this) = std::forward<InputType>(value);
            return *this;
        }

        // active type is NOT empty & NOT the input type destroy the existing data there
        if (_active_index != empty_index) {
            _data.destroy(_active_index, 0);
            _active_index = empty_index;
        }

        // now create the value of the input type is the destroyed slot
        _data.template create_from_value<target_index>(std::forward<InputType>(value));
        _active_index = target_index;

        return *this;
    }

    // swapping
    void swap(variant<AlternativeTypes...>& other) noexcept {
        if (this == std::addressof(other)) return;

        if (_active_index == other._active_index) {
            if (_active_index != empty_index) _data.swap_active(other._data, _active_index, 0);
            return;
        }

        variant temp(std::move(*this));
        *this = std::move(other);
        other = std::move(temp);
    }

    // destructor
    constexpr ~variant() {
        if (_active_index != empty_index) _data.destroy(_active_index, 0);
    }

    constexpr std::size_t index() const noexcept { return static_cast<std::size_t>(_active_index); }

    // access by index

    // const
    template <std::size_t I, typename... OtherAlternativeTypes>
    friend constexpr decltype(auto) get(const variant<OtherAlternativeTypes...>& v);

    // non-const
    template <std::size_t I, typename... OtherAlternativeTypes>
    friend constexpr decltype(auto) get(variant<OtherAlternativeTypes...>& v);

    // access by type

    // const
    template <typename TargetType, typename... OtherAlternativeTypes>
    friend constexpr decltype(auto) get(const variant<OtherAlternativeTypes...>& v);

    // non-const
    template <typename TargetType, typename... OtherAlternativeTypes>
    friend constexpr decltype(auto) get(variant<OtherAlternativeTypes...>& v);

    constexpr bool valueless_by_exception() const noexcept { return _active_index == empty_index; }
};

template <std::size_t I, typename... AlternativeTypes>
constexpr decltype(auto) get(const variant<AlternativeTypes...>& value) {
    static_assert(I < sizeof...(AlternativeTypes), "alternative index out of bounds");
    if constexpr (I < sizeof...(AlternativeTypes)) {
        if (I != value.index()) throw std::bad_variant_access();
        return value.template access<I>(value._data);
    }
}

template <std::size_t I, typename... AlternativeTypes>
constexpr decltype(auto) get(variant<AlternativeTypes...>& value) {
    static_assert(I < sizeof...(AlternativeTypes), "alternative index out of bounds");
    if constexpr (I < sizeof...(AlternativeTypes)) {
        if (I != value.index()) throw std::bad_variant_access();
        return value.template access<I>(value._data);
    }
}

template <typename TargetType, typename... AlternativeTypes>
constexpr decltype(auto) get(const variant<AlternativeTypes...>& value) {
    constexpr auto matches = (0 + ... + std::is_same_v<TargetType, AlternativeTypes>);
    static_assert(matches == 1, "type must appear exactly once");
    if constexpr (matches == 1) {
        constexpr auto index =
            variant<AlternativeTypes...>::template type_index<TargetType, AlternativeTypes...>();
        return ravig31::get<index>(value);
    }
}

template <typename TargetType, typename... AlternativeTypes>
constexpr decltype(auto) get(variant<AlternativeTypes...>& value) {
    constexpr auto matches = (0 + ... + std::is_same_v<TargetType, AlternativeTypes>);
    static_assert(matches == 1, "type must appear exactly once");
    if constexpr (matches == 1) {
        constexpr auto index =
            variant<AlternativeTypes...>::template type_index<TargetType, AlternativeTypes...>();
        return ravig31::get<index>(value);
    }
}

}  // namespace ravig31
