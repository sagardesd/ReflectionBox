#include <meta>
#include <print>
#include <string_view>
#include <type_traits>

template <typename T>
consteval std::string_view type_name() {
    return std::meta::identifier_of(^^T);
}

template <typename T>
consteval auto member_infos() {
    return std::define_static_array(
        std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current())
    );
}

template <typename T>
void print_members() {
    std::println("Members of '{}':", type_name<T>());

    constexpr auto members = member_infos<T>();

    template for (constexpr std::meta::info m : members) {
        std::println("  {}", std::meta::identifier_of(m));
    }
}

template <typename E>
    requires std::is_enum_v<E>
consteval std::string_view enum_name(E value) {
    constexpr auto enumerators = std::define_static_array(
        std::meta::enumerators_of(^^E));

    template for (constexpr std::meta::info e : enumerators) {
        if (value == [:e:]) {
            return std::meta::identifier_of(e);
        }
    }
    return "<unknown>";
}

template <typename E>
    requires std::is_enum_v<E>
std::string_view runtime_enum_name(E value) {
    constexpr auto enumerators = std::define_static_array(
        std::meta::enumerators_of(^^E));

    template for (constexpr std::meta::info e : enumerators) {
        if (value == [:e:]) {
            return std::meta::identifier_of(e);
        }
    }
    return "<unknown>";
}

template <typename T>
void print_struct(const T& s) {
    constexpr auto members = member_infos<T>();

    std::print("{{ ");
    bool first = true;

    template for (constexpr std::meta::info m : members) {
        if (!first) std::print(", ");
        std::print("{}: ", std::meta::identifier_of(m));
        if constexpr (std::is_enum_v<typename [:std::meta::type_of(m):]>) {
            std::print("{}", runtime_enum_name(s.[:m:]));
        } else {
            std::print("{}", s.[:m:]);
        }
        first = false;
    }

    std::println(" }}");
}

enum class Direction { North, South, East, West };

struct Point {
    double x, y, z;
};

struct Particle {
    std::string_view name;
    double mass;
    int charge;
    Direction spin_direction;
};

int main() {
    print_members<Particle>();

    std::println("Enum values:");
    std::println("  Direction::{}", enum_name(Direction::North));
    std::println("  Direction::{}", enum_name(Direction::South));
    std::println("  Direction::{}", enum_name(Direction::East));
    std::println("  Direction::{}", enum_name(Direction::West));

    Particle p{"electron", 0.000511, -1, Direction::North};
    std::print("Particle: ");
    print_struct(p);

    constexpr std::size_t n = member_infos<Particle>().size();
    std::println("Particle has {} fields (known at compile time)", n);
}
