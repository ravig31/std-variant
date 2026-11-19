#include <cassert>
#include <memory>
#include <type_traits>
#include <utility>
#include <variant>

template <typename... AlternativeTypes>
class Variant {
    static constexpr int empty_index = -1;

   private:
    struct EmptyStorageTag {};

    template <typename FirstType, typename... RemainingTypes>
    union Values {
        FirstType head;
        Values<RemainingTypes...> tail;

        Values() : head{} {};

        Values(EmptyStorageTag) {};

        template <typename InputType>
        Values(std::in_place_index_t<0>, InputType&& value)
            : head{std::forward<InputType>(value)} {}

        template <std::size_t I, typename InputType>
            requires(I > 0)
        Values(std::in_place_index_t<I>, InputType&& value)
            : tail{std::in_place_index<I - 1>, std::forward<InputType>(value)} {}

        constexpr ~Values() {};

        // no template for Values means its the current specilisation, which makse sense here as we
        // are just copying
        constexpr void
        create_from_copy(const Values& source, int current_active_index, int travel_count) {
            if (travel_count == current_active_index) {
                std::construct_at(std::addressof(head), source.head);
                return;
            }

            std::construct_at(std::addressof(tail), EmptyStorageTag{});
            tail.create_from_copy(source.tail, current_active_index, travel_count + 1);
        }

        constexpr void
        assign_from_copy(const Values& source, int current_active_index, int travel_count) {
            if (travel_count == current_active_index) {
                head = source.head;
                return;
            }

            tail.assign_from_copy(source.tail, current_active_index, travel_count + 1);
        }

        constexpr void
        create_from_move(Values& source, int current_active_index, int travel_count) {
            if (travel_count == current_active_index) {
                std::construct_at(std::addressof(head), std::move(source.head));
                return;
            }

            std::construct_at(std::addressof(tail), EmptyStorageTag{});
            tail.create_from_move(source.tail, current_active_index, travel_count + 1);
        }

        constexpr void
        assign_from_move(Values& source, int current_active_index, int travel_count) {
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

        constexpr void swap_active(Values& other, int current_active_index, int travel_count) {
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
    union Values<LastType> {
        LastType head;

        Values() : head{} {};

        Values(EmptyStorageTag) {};

        template <typename InputType>
        Values(std::in_place_index_t<0>, InputType&& value)
            : head{std::forward<InputType>(value)} {}

        constexpr ~Values() {};

        constexpr void
        create_from_copy(const Values& source, int current_active_index, int travel_count) {
            assert(travel_count == current_active_index);
            std::construct_at(std::addressof(head), source.head);
            return;
        }

        constexpr void
        assign_from_copy(const Values& source, int current_active_index, int travel_count) {
            assert(travel_count == current_active_index);
            head = source.head;
        }

        constexpr void
        create_from_move(Values& source, int current_active_index, int travel_count) {
            assert(travel_count == current_active_index);
            std::construct_at(std::addressof(head), std::move(source.head));
            return;
        }

        constexpr void
        assign_from_move(Values& source, int current_active_index, int travel_count) {
            assert(travel_count == current_active_index);
            head = std::move(source.head);
        }

        template <std::size_t I, typename InputType>
        constexpr void create_from_value(InputType&& value) {
            static_assert(I == 0, "alternative index out of bounds");
            std::construct_at(std::addressof(head), std::forward<InputType>(value));
        }

        constexpr void swap_active(Values& other, int current_active_index, int travel_count) {
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

    template <int I, typename StorageType>
    auto& access(StorageType& data) const {
        if constexpr (I == 0)
            return data.head;
        else
            return access<I - 1>(data.tail);
    }

    int _active_index;

    Values<AlternativeTypes...> _data;

   public:
    Variant() : _active_index{}, _data{} {}

    // copy constructor
    Variant(const Variant& other) : _active_index{other._active_index}, _data{EmptyStorageTag{}} {
        if (_active_index != empty_index) _data.create_from_copy(other._data, _active_index, 0);
    }

    // copy assignment
    Variant& operator=(const Variant& other) {
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
    Variant(Variant&& other) : _active_index{other._active_index}, _data{EmptyStorageTag{}} {
        if (_active_index != empty_index) _data.create_from_move(other._data, _active_index, 0);
    }

    // move assignment
    Variant& operator=(Variant&& other) {
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
        requires(!std::is_same_v<std::remove_cvref_t<InputType>, Variant>)
    constexpr explicit Variant(InputType&& value)
        : _active_index(type_index<std::remove_cvref_t<InputType>, AlternativeTypes...>()),
          _data(
              std::in_place_index_t<
                  type_index<std::remove_cvref_t<InputType>, AlternativeTypes...>()>(),
              std::forward<InputType>(value)) {}

    // converting assignment
    template <typename InputType>
        requires(!std::is_same_v<std::remove_cvref_t<InputType>, Variant>)
    constexpr Variant<AlternativeTypes...>& operator=(InputType&& value) {
        constexpr auto target_index =
            type_index<std::remove_cvref_t<InputType>, AlternativeTypes...>();

        // active type is already the input type, just slot in the new value
        if (_active_index == target_index) {
            get<target_index>() = std::forward<InputType>(value);
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
    void swap(Variant<AlternativeTypes...>& other) noexcept {
        if (this == std::addressof(other)) return;

        if (_active_index == other._active_index) {
            if (_active_index != empty_index) _data.swap_active(other._data, _active_index, 0);
            return;
        }

        Variant temp(std::move(*this));
        *this = std::move(other);
        other = std::move(temp);
    }

    // destructor
    constexpr ~Variant() {
        if (_active_index != empty_index) _data.destroy(_active_index, 0);
    }

    int index() const { return _active_index; }

    // access by index
    template <int I>
    auto& get() {
        if (I != _active_index) throw std::bad_variant_access();
        static_assert(I >= 0 && I < sizeof...(AlternativeTypes), "alternative index out of bounds");

        return access<I>(_data);
    }

    template <int I>
    const auto& get() const {
        if (I != _active_index) throw std::bad_variant_access();
        static_assert(I >= 0 && I < sizeof...(AlternativeTypes), "alternative index out of bounds");

        return access<I>(_data);
    }

    // access by type
    template <typename TargetType>
    auto& get() {
        static_assert(
            (0 + ... + std::is_same_v<TargetType, AlternativeTypes>) == 1,
            "type must appear exactly once");
        constexpr auto index = type_index<TargetType, AlternativeTypes...>();

        return get<index>();
    }

    template <typename TargetType>
    const auto& get() const {
        static_assert(
            (0 + ... + std::is_same_v<TargetType, AlternativeTypes>) == 1,
            "type must appear exactly once");
        constexpr auto index = type_index<TargetType, AlternativeTypes...>();

        return get<index>();
    }

    constexpr bool valueless_by_exception() const noexcept { return _active_index == empty_index; }
};

struct Tracked {
    inline static int destructions = 0;

    ~Tracked() { ++destructions; }
};

struct ThrowOnCopy {
    ThrowOnCopy() = default;

    ThrowOnCopy(ThrowOnCopy&&) = default;

    ThrowOnCopy(const ThrowOnCopy&) { throw 42; }

    ThrowOnCopy& operator=(const ThrowOnCopy&) = default;
};

int main() {
    Variant<int, std::string> value(std::string{"hello"});

    value.get<std::string>() = "changed";
    assert(value.get<1>() == "changed");

    const auto& view = value;
    assert(view.get<std::string>() == "changed");

    static_assert(std::is_same_v<decltype(view.get<std::string>()), const std::string&>);
}
