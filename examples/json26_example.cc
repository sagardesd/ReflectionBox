// json26_example.cc — Full round-trip demo for json26.hpp
// bazel run //:json26_example

#include "json26.hpp"
#include <cassert>
#include <map>
#include <print>
#include <vector>

enum class Color { Red, Green, Blue };

struct Address {
    std::string street;
    std::string city;
    int         zip{};
};

struct Person {
    std::string              name;
    int                      age{};
    double                   score{};
    bool                     active{};
    Color                    favorite_color{};
    std::optional<Address>   address;
    std::vector<std::string> tags;
};

int main() {
    Person alice{
        .name           = "Alice",
        .age            = 30,
        .score          = 9.5,
        .active         = true,
        .favorite_color = Color::Blue,
        .address        = Address{"123 Main St", "Wonderland", 12345},
        .tags           = {"admin", "user"},
    };

    // ── Serialize ─────────────────────────────────────────────
    std::string json = json::serialize(alice, /*indent=*/2);
    std::println("=== Serialized ===\n{}\n", json);

    // ── Round-trip ────────────────────────────────────────────
    Person bob = json::deserialize<Person>(json);
    assert(bob.name            == alice.name);
    assert(bob.age             == alice.age);
    assert(bob.score           == alice.score);
    assert(bob.active          == alice.active);
    assert(bob.favorite_color  == Color::Blue);
    assert(bob.address.has_value());
    assert(bob.address->city   == "Wonderland");
    assert(bob.address->zip    == 12345);
    assert(bob.tags            == alice.tags);
    std::println("Round-trip assertion PASSED");

    // ── Null optional ─────────────────────────────────────────
    Person charlie{.name = "Charlie", .age = 25};
    auto   cjson = json::serialize(charlie);
    Person charlie2 = json::deserialize<Person>(cjson);
    assert(!charlie2.address.has_value());
    std::println("Null optional PASSED");

    // ── std::map round-trip ───────────────────────────────────
    std::map<std::string, int> scores{{"a", 1}, {"b", 2}, {"c", 3}};
    auto scores_json = json::serialize(scores);
    auto scores2     = json::deserialize<std::map<std::string, int>>(scores_json);
    assert(scores == scores2);
    std::println("std::map round-trip PASSED: {}", scores_json);

    // ── Direct value manipulation ─────────────────────────────
    json::value v = json::parse(R"({"x":1,"y":[2,3,4]})");
    v["z"]        = json::value{true};
    v["y"][0]     = json::value{99};
    std::println("Mutated value: {}", json::stringify(v, 2));

    // ── Parse edge cases ─────────────────────────────────────
    assert(json::parse("null").is_null());
    assert(json::parse("true").as<bool>() == true);
    assert(json::parse("42").as<json::int_t>() == 42);
    assert(json::parse("3.14").as<json::float_t>() == 3.14);
    assert(json::parse(R"("hello\nworld")").as<std::string>() == "hello\nworld");
    assert(json::parse("[]").is_array());
    assert(json::parse("{}").is_object());
    std::println("Parse edge cases PASSED");

    std::println("\nAll tests PASSED.");
}
