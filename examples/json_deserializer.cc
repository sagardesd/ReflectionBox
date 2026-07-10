// json_deserializer.cc — reflection-powered JSON deserializer demo
// Parses a mock JSON payload into a strongly-typed C++ struct
// with zero macros or base classes.
//
//   ./reflect examples/json_deserializer.cc

#include <meta>
#include <any>
#include <array>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// ── Mock JSON payload (no external parser needed for the demo) ─
struct JsonObject {
    std::map<std::string, std::any> fields;

    bool contains(std::string_view key) const {
        return fields.find(std::string(key)) != fields.end();
    }
    template <typename T>
    T get(std::string_view key) const {
        return std::any_cast<T>(fields.at(std::string(key)));
    }
    JsonObject get_child(std::string_view key) const {
        return std::any_cast<JsonObject>(fields.at(std::string(key)));
    }
    std::vector<JsonObject> get_array(std::string_view key) const {
        return std::any_cast<std::vector<JsonObject>>(fields.at(std::string(key)));
    }

    static JsonObject create_mock_payload() {
        JsonObject boss;
        boss.fields["name"]   = std::string("General Radahn");
        boss.fields["health"] = 5000;
        boss.fields["shield"] = 150;
        JsonObject wp1; wp1.fields["x"] = 100.5f; wp1.fields["y"] = 200.0f;
        JsonObject wp2; wp2.fields["x"] = 150.0f; wp2.fields["y"] = 250.5f;
        boss.fields["patrol_path"] = std::vector<JsonObject>{wp1, wp2};
        return boss;
    }
};

// ── Type traits ───────────────────────────────────────────────
template <typename T> struct is_optional                      : std::false_type {};
template <typename T> struct is_optional<std::optional<T>>   : std::true_type  {};
template <typename T> struct is_vector                        : std::false_type {};
template <typename T, typename A>
struct is_vector<std::vector<T, A>>                           : std::true_type  {};
template <typename T> struct inner;
template <typename T> struct inner<std::optional<T>>         { using type = T; };
template <typename T, typename A>
struct inner<std::vector<T, A>>                               { using type = T; };

// ── Target structs — no macros, no base classes ───────────────
struct Vector2 { float x, y; };

struct Enemy {
    std::string          name;
    int                  health;
    std::optional<int>   shield;
    std::vector<Vector2> patrol_path;
};

// ── Reflection-powered deserializer ──────────────────────────
template <typename T>
T from_json(const JsonObject& json) {
    T obj{};
    template for (constexpr auto mem : std::meta::nonstatic_data_members_of(^T)) {
        constexpr auto    name      = std::meta::identifier_of(mem);
        using             FieldType = typename[:std::meta::type_of(mem):];

        if (!json.contains(name)) continue;

        if constexpr (is_optional<FieldType>::value) {
            obj.[:mem:] = json.get<typename inner<FieldType>::type>(name);
        } else if constexpr (is_vector<FieldType>::value) {
            for (const auto& item : json.get_array(name))
                obj.[:mem:].push_back(from_json<typename inner<FieldType>::type>(item));
        } else if constexpr (std::meta::is_class_type(^FieldType) &&
                             !std::is_same_v<FieldType, std::string>) {
            obj.[:mem:] = from_json<FieldType>(json.get_child(name));
        } else {
            obj.[:mem:] = json.get<FieldType>(name);
        }
    }
    return obj;
}

int main() {
    JsonObject payload = JsonObject::create_mock_payload();
    Enemy boss = from_json<Enemy>(payload);

    std::cout << "Boss:      " << boss.name   << "\n";
    std::cout << "Health:    " << boss.health  << "\n";
    if (boss.shield) std::cout << "Shield:    " << *boss.shield << "\n";
    std::cout << "Waypoints: " << boss.patrol_path.size() << "\n";
    for (std::size_t i = 0; i < boss.patrol_path.size(); ++i)
        std::cout << "  [" << i << "] x=" << boss.patrol_path[i].x
                           << " y=" << boss.patrol_path[i].y << "\n";
}
