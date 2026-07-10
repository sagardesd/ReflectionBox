// Minimal P2996 reflection smoke test — run with:
//   bazel run //:reflection_demo
// or:
//   clang++ -std=c++26 -freflection -stdlib=libc++ examples/reflection_demo.cc -o demo

#include <meta>
#include <print>
#include <string_view>

// ── Print all member names of a struct at compile time ────────
template <typename T>
void print_members() {
    std::println("Members of '{}':", std::meta::identifier_of(^T));
    template for (constexpr auto m : std::meta::nonstatic_data_members_of(^T)) {
        std::println("  {} : {}", std::meta::identifier_of(m),
                     std::meta::identifier_of(std::meta::type_of(m)));
    }
}

// ── Enum ↔ string via reflection ─────────────────────────────
template <typename E>
    requires std::is_enum_v<E>
constexpr std::string_view enum_name(E value) {
    template for (constexpr auto e : std::meta::enumerators_of(^E)) {
        if (value == [:e:]) return std::meta::identifier_of(e);
    }
    return "<unknown>";
}

// ── Generic struct printer ────────────────────────────────────
template <typename T>
void print_struct(const T& s) {
    std::print("{{ ");
    bool first = true;
    template for (constexpr auto m : std::meta::nonstatic_data_members_of(^T)) {
        if (!first) std::print(", ");
        std::print("{}: {}", std::meta::identifier_of(m), s.[:m:]);
        first = false;
    }
    std::println(" }}");
}

// ── Sample types ──────────────────────────────────────────────
enum class Direction { North, South, East, West };

struct Point { double x, y, z; };

struct Particle {
    std::string_view name;
    double           mass;   // GeV/c²
    int              charge;
    Direction        spin_direction;
};

int main() {
    // Member introspection
    print_members<Particle>();

    // Enum reflection
    for (auto d : {Direction::North, Direction::South, Direction::East, Direction::West})
        std::println("  Direction::{}", enum_name(d));

    // Generic struct print
    Particle p{"electron", 0.000511, -1, Direction::North};
    print_struct(p);

    // Compile-time member count
    constexpr std::size_t n = std::meta::nonstatic_data_members_of(^Particle).size();
    std::println("Particle has {} fields (known at compile time)", n);
}
