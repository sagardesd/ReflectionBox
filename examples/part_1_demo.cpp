#include <meta>
#include <print>
#include <string_view>

template <typename T>
consteval auto member_infos() {
    return std::define_static_array(
        std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current())
    );
}

template <typename T>
void print_struct(const T& s) {
    constexpr auto members = member_infos<T>();

    std::print("{{ ");
    bool first = true;

    template for (constexpr std::meta::info m : members) {
        if (!first) std::print(", ");
        std::print("{}: {}", std::meta::identifier_of(m), s.[:m:]);
        first = false;
    }

    std::println(" }}");
}

struct Point {
    int x;
    int y;
    int z;
};

int main() {
    Point p{1, 2, 3};
    print_struct(p);
}
