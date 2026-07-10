// json26_test.cc — bazel test //:json26_test
// Returns 0 on success, non-zero on any assertion failure.
#include "json26.hpp"
#include <cassert>
#include <cstdlib>
#include <print>
#include <vector>
#include <map>

// ── Test helpers ──────────────────────────────────────────────
static int pass_count = 0;
static int fail_count = 0;

#define TEST(name, expr) do { \
    if (expr) { ++pass_count; std::println("[PASS] {}", name); } \
    else      { ++fail_count; std::println("[FAIL] {}", name); } \
} while(0)

// ── Fixture types ─────────────────────────────────────────────
enum class Status { Active, Inactive, Pending };

struct Inner { int a; double b; };
struct Outer { std::string name; Inner inner; std::optional<Inner> maybe; };

int main() {
    // ── Primitives ────────────────────────────────────────────
    TEST("null rt",    json::deserialize<json::null_t>("null")   == json::null_t{});
    TEST("bool true",  json::deserialize<bool>("true")           == true);
    TEST("bool false", json::deserialize<bool>("false")          == false);
    TEST("int",        json::deserialize<int>("42")              == 42);
    TEST("int neg",    json::deserialize<int>("-7")              == -7);
    TEST("double",     json::deserialize<double>("3.14")         == 3.14);
    TEST("string",     json::deserialize<std::string>(R"("hi")") == "hi");

    // ── Escape sequences ──────────────────────────────────────
    auto esc = json::deserialize<std::string>(R"("a\nb\tc")");
    TEST("escape \\n\\t", esc == "a\nb\tc");

    // ── Arrays ────────────────────────────────────────────────
    auto v = json::deserialize<std::vector<int>>("[1,2,3]");
    TEST("array size", v.size() == 3);
    TEST("array[0]",   v[0] == 1);
    TEST("array[2]",   v[2] == 3);

    // ── Maps ──────────────────────────────────────────────────
    std::map<std::string,int> m{{"x",10},{"y",20}};
    TEST("map rt", json::deserialize<std::map<std::string,int>>(json::serialize(m)) == m);

    // ── Enums ─────────────────────────────────────────────────
    TEST("enum ser",   json::serialize(Status::Active) == R"("Active")");
    TEST("enum deser", json::deserialize<Status>(R"("Pending")") == Status::Pending);

    // ── Optional ──────────────────────────────────────────────
    std::optional<int> some = 42, none = std::nullopt;
    TEST("opt some", json::deserialize<std::optional<int>>(json::serialize(some)) == 42);
    TEST("opt none", json::deserialize<std::optional<int>>(json::serialize(none)) == std::nullopt);

    // ── Nested struct ─────────────────────────────────────────
    Outer o{"test", {1, 2.5}, std::nullopt};
    Outer o2 = json::deserialize<Outer>(json::serialize(o, 0));
    TEST("nested name",    o2.name        == o.name);
    TEST("nested inner.a", o2.inner.a     == o.inner.a);
    TEST("nested inner.b", o2.inner.b     == o.inner.b);
    TEST("nested maybe",   !o2.maybe.has_value());

    Outer o3{"x", {0,0}, Inner{7, 3.14}};
    Outer o4 = json::deserialize<Outer>(json::serialize(o3));
    TEST("nested maybe val", o4.maybe.has_value() && o4.maybe->a == 7);

    // ── Value API ─────────────────────────────────────────────
    json::value jv = json::parse(R"({"a":1,"b":[true,null]})");
    TEST("obj.a",       jv["a"].as<json::int_t>() == 1);
    TEST("obj.b[0]",    jv["b"][0].as<bool>() == true);
    TEST("obj.b[1]",    jv["b"][1].is_null());
    TEST("contains",    jv.contains("a"));
    TEST("!contains",   !jv.contains("z"));

    // ── Pretty / compact round-trip ───────────────────────────
    std::string compact = json::stringify(jv, 0);
    std::string pretty  = json::stringify(jv, 2);
    TEST("reparse compact", json::parse(compact) == jv);
    TEST("reparse pretty",  json::parse(pretty)  == jv);

    // ── Error handling ────────────────────────────────────────
    bool threw = false;
    try { json::parse("{bad}"); } catch (const std::runtime_error&) { threw = true; }
    TEST("parse error",  threw);

    bool type_threw = false;
    try { json::from_value<int>(json::value{"hello"}); }
    catch (const std::runtime_error&) { type_threw = true; }
    TEST("type mismatch", type_threw);

    // ── Summary ───────────────────────────────────────────────
    std::println("\n{} passed, {} failed.", pass_count, fail_count);
    return fail_count == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
