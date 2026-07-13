// json26.hpp — Header-only JSON serializer/deserializer using C++26 reflection (P2996)
// Compile with: -std=c++26 -freflection  (Clang trunk with P2996 patch)
// Standard header: <meta>; some builds use <experimental/meta>
#pragma once

#include <algorithm>
#include <cassert>
#include <charconv>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <meta>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

namespace json {


// Core value type
struct value;

using null_t   = std::monostate;
using bool_t   = bool;
using int_t    = std::int64_t;
using float_t  = double;
using string_t = std::string;
using array_t  = std::vector<value>;
using object_t = std::map<std::string, value, std::less<>>;

struct value {
    using storage_t = std::variant<null_t, bool_t, int_t, float_t, string_t, array_t, object_t>;
    storage_t v_;

    // --- construction ---
    value()                              : v_(null_t{})        {}
    value(null_t)                        : v_(null_t{})        {}
    value(bool b)                        : v_(bool_t{b})       {}
    value(int_t i)                       : v_(i)               {}
    value(float_t f)                     : v_(f)               {}
    value(std::string s)                 : v_(std::move(s))    {}
    value(std::string_view sv)           : v_(std::string{sv}) {}
    value(const char* s)                 : v_(std::string{s})  {}
    value(array_t a)                     : v_(std::move(a))    {}
    value(object_t o)                    : v_(std::move(o))    {}

    template <std::integral I>
        requires (!std::same_as<I, bool>)
    value(I i) : v_(static_cast<int_t>(i)) {}

    template <std::floating_point F>
    value(F f) : v_(static_cast<float_t>(f)) {}

    // type queries
    bool is_null()   const noexcept { return std::holds_alternative<null_t>(v_); }
    bool is_bool()   const noexcept { return std::holds_alternative<bool_t>(v_); }
    bool is_int()    const noexcept { return std::holds_alternative<int_t>(v_); }
    bool is_float()  const noexcept { return std::holds_alternative<float_t>(v_); }
    bool is_number() const noexcept { return is_int() || is_float(); }
    bool is_string() const noexcept { return std::holds_alternative<string_t>(v_); }
    bool is_array()  const noexcept { return std::holds_alternative<array_t>(v_); }
    bool is_object() const noexcept { return std::holds_alternative<object_t>(v_); }

    // typed access (throws std::bad_variant_access on mismatch)
    template <typename T>       T& as()       { return std::get<T>(v_); }
    template <typename T> const T& as() const { return std::get<T>(v_); }

    // object accessors
    value& operator[](std::string_view k) {
        return as<object_t>()[std::string{k}];
    }
    const value& operator[](std::string_view k) const {
        return as<object_t>().at(std::string{k});
    }
    bool contains(std::string_view k) const {
        return is_object() && as<object_t>().count(std::string{k});
    }

    // array accessors
    value&       operator[](std::size_t i)       { return as<array_t>()[i]; }
    const value& operator[](std::size_t i) const { return as<array_t>()[i]; }

    std::size_t size() const noexcept {
        if (is_array())  return as<array_t>().size();
        if (is_object()) return as<object_t>().size();
        return 0;
    }
    bool empty() const noexcept { return size() == 0; }

    // equality
    bool operator==(const value&) const = default;
};

// Convenience factories
inline value object(object_t o = {}) { return value{std::move(o)}; }
inline value array(array_t a  = {}) { return value{std::move(a)}; }


// Type-category concepts
template <typename T>
concept optional_like = requires(T t) {
    typename T::value_type;
    { t.has_value() } -> std::convertible_to<bool>;
    { *t } -> std::convertible_to<typename T::value_type>;
};

template <typename T>
concept map_like = requires {
    typename T::key_type;
    typename T::mapped_type;
    requires std::convertible_to<typename T::key_type, std::string>;
} && requires(T t) { t.begin(); t.end(); };

// sequence containers but not strings/maps
template <typename T>
concept array_like = requires(T t) {
    typename T::value_type;
    t.begin(); t.end(); t.size();
} && !std::same_as<T, std::string>
  && !std::same_as<T, std::string_view>
  && !map_like<T>;

// user aggregate (struct/class): use reflection
template <typename T>
concept reflectable_aggregate =
    std::is_aggregate_v<T>
    && !std::is_array_v<T>
    && !array_like<T>
    && !map_like<T>
    && !optional_like<T>
    && !std::same_as<T, std::string>
    && !std::same_as<T, std::string_view>;


// Error helper
namespace detail {

[[noreturn]] inline void type_mismatch(std::string_view expected, const value& got) {
    const char* actual =
        got.is_null()   ? "null"   :
        got.is_bool()   ? "bool"   :
        got.is_int()    ? "int"    :
        got.is_float()  ? "float"  :
        got.is_string() ? "string" :
        got.is_array()  ? "array"  : "object";
    throw std::runtime_error(
        std::string{"json: expected "}.append(expected).append(", got ").append(actual));
}

} // namespace detail


// to_value — C++ type → json::value
template <typename T>
value to_value(const T& t) {
    if constexpr (std::same_as<T, null_t> || std::same_as<T, std::nullptr_t>) {
        return value{};

    } else if constexpr (std::same_as<T, bool>) {
        return value{t};

    } else if constexpr (std::integral<T>) {
        return value{static_cast<int_t>(t)};

    } else if constexpr (std::floating_point<T>) {
        return value{static_cast<float_t>(t)};

    } else if constexpr (std::same_as<T, std::string>  ||
                         std::same_as<T, std::string_view>) {
        return value{std::string{t}};

    } else if constexpr (std::same_as<T, const char*>) {
        return value{std::string{t}};

    } else if constexpr (std::is_enum_v<T>) {
        // Serialize as string name; fall back to integer if no match
        template for (constexpr auto e : std::meta::enumerators_of(^T)) {
            if (t == [:e:])
                return value{std::string{std::meta::identifier_of(e)}};
        }
        return value{static_cast<int_t>(t)};

    } else if constexpr (optional_like<T>) {
        if (!t.has_value()) return value{};
        return to_value(*t);

    } else if constexpr (array_like<T>) {
        array_t arr;
        if constexpr (requires { arr.reserve(t.size()); })
            arr.reserve(t.size());
        for (const auto& item : t)
            arr.push_back(to_value(item));
        return value{std::move(arr)};

    } else if constexpr (map_like<T>) {
        object_t obj;
        for (const auto& [k, v] : t)
            obj[std::string{k}] = to_value(v);
        return value{std::move(obj)};

    } else if constexpr (reflectable_aggregate<T>) {
        object_t obj;
        template for (constexpr auto m : std::meta::nonstatic_data_members_of(^T)) {
            obj[std::string{std::meta::identifier_of(m)}] = to_value(t.[:m:]);
        }
        return value{std::move(obj)};

    } else {
        static_assert(false, "json::to_value: unsupported type");
    }
}


// from_value — json::value → C++ type
template <typename T>
T from_value(const value& v) {
    if constexpr (std::same_as<T, null_t>) {
        if (!v.is_null()) detail::type_mismatch("null", v);
        return null_t{};

    } else if constexpr (std::same_as<T, std::nullptr_t>) {
        if (!v.is_null()) detail::type_mismatch("null", v);
        return nullptr;

    } else if constexpr (std::same_as<T, bool>) {
        if (!v.is_bool()) detail::type_mismatch("bool", v);
        return v.as<bool_t>();

    } else if constexpr (std::integral<T>) {
        if (v.is_int())   return static_cast<T>(v.as<int_t>());
        if (v.is_float()) return static_cast<T>(v.as<float_t>());
        detail::type_mismatch("integer", v);

    } else if constexpr (std::floating_point<T>) {
        if (v.is_float()) return static_cast<T>(v.as<float_t>());
        if (v.is_int())   return static_cast<T>(v.as<int_t>());
        detail::type_mismatch("float", v);

    } else if constexpr (std::same_as<T, std::string>) {
        if (!v.is_string()) detail::type_mismatch("string", v);
        return v.as<string_t>();

    } else if constexpr (std::is_enum_v<T>) {
        if (v.is_string()) {
            const auto& s = v.as<string_t>();
            template for (constexpr auto e : std::meta::enumerators_of(^T)) {
                if (s == std::meta::identifier_of(e))
                    return [:e:];
            }
            throw std::runtime_error("json: unknown enumerator: " + s);
        }
        if (v.is_int()) return static_cast<T>(v.as<int_t>());
        detail::type_mismatch("string or int (for enum)", v);

    } else if constexpr (optional_like<T>) {
        if (v.is_null()) return T{std::nullopt};
        return T{from_value<typename T::value_type>(v)};

    } else if constexpr (array_like<T>) {
        if (!v.is_array()) detail::type_mismatch("array", v);
        T result;
        if constexpr (requires { result.reserve(0uz); })
            result.reserve(v.as<array_t>().size());
        for (const auto& item : v.as<array_t>())
            result.push_back(from_value<typename T::value_type>(item));
        return result;

    } else if constexpr (map_like<T>) {
        if (!v.is_object()) detail::type_mismatch("object", v);
        T result;
        for (const auto& [k, val] : v.as<object_t>())
            result[typename T::key_type{k}] = from_value<typename T::mapped_type>(val);
        return result;

    } else if constexpr (reflectable_aggregate<T>) {
        if (!v.is_object()) detail::type_mismatch("object", v);
        const auto& obj = v.as<object_t>();
        T result{};
        template for (constexpr auto m : std::meta::nonstatic_data_members_of(^T)) {
            constexpr std::string_view name = std::meta::identifier_of(m);
            if (auto it = obj.find(std::string{name}); it != obj.end())
                result.[:m:] = from_value<typename[:std::meta::type_of(m):]>(it->second);
        }
        return result;

    } else {
        static_assert(false, "json::from_value: unsupported type");
    }
}


// stringify — json::value → std::string
namespace detail {

inline void escape(std::string& out, std::string_view s) {
    out += '"';
    for (unsigned char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b";  break;
            case '\f': out += "\\f";  break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", unsigned(c));
                    out += buf;
                } else {
                    out += static_cast<char>(c);
                }
        }
    }
    out += '"';
}

inline void emit(const value& v, std::string& out, int indent, int depth) {
    std::visit([&](auto const& x) {
        using X = std::decay_t<decltype(x)>;

        if constexpr (std::same_as<X, null_t>) {
            out += "null";

        } else if constexpr (std::same_as<X, bool_t>) {
            out += x ? "true" : "false";

        } else if constexpr (std::same_as<X, int_t>) {
            out += std::to_string(x);

        } else if constexpr (std::same_as<X, float_t>) {
            if (std::isnan(x) || std::isinf(x)) {
                out += "null"; // JSON spec forbids NaN / Infinity
            } else {
                char buf[32];
                auto [p, ec] = std::to_chars(buf, buf + sizeof(buf), x,
                                             std::chars_format::general, 17);
                out.append(buf, p);
            }

        } else if constexpr (std::same_as<X, string_t>) {
            escape(out, x);

        } else if constexpr (std::same_as<X, array_t>) {
            if (x.empty()) { out += "[]"; return; }
            out += '[';
            for (std::size_t i = 0; i < x.size(); ++i) {
                if (i) out += ',';
                if (indent > 0) {
                    out += '\n';
                    out.append((depth + 1) * indent, ' ');
                }
                emit(x[i], out, indent, depth + 1);
            }
            if (indent > 0) { out += '\n'; out.append(depth * indent, ' '); }
            out += ']';

        } else if constexpr (std::same_as<X, object_t>) {
            if (x.empty()) { out += "{}"; return; }
            out += '{';
            bool first = true;
            for (const auto& [k, w] : x) {
                if (!first) out += ',';
                if (indent > 0) {
                    out += '\n';
                    out.append((depth + 1) * indent, ' ');
                }
                escape(out, k);
                out += ':';
                if (indent > 0) out += ' ';
                emit(w, out, indent, depth + 1);
                first = false;
            }
            if (indent > 0) { out += '\n'; out.append(depth * indent, ' '); }
            out += '}';
        }
    }, v.v_);
}

} // namespace detail

inline std::string stringify(const value& v, int indent = 0) {
    std::string out;
    detail::emit(v, out, indent, 0);
    return out;
}


// parse — std::string_view → json::value
namespace detail {

struct parser {
    const char* cur;
    const char* end;

    [[nodiscard]] char peek()  const noexcept { return cur < end ? *cur : '\0'; }
    [[nodiscard]] char take() {
        if (cur >= end) throw std::runtime_error("json::parse: unexpected end of input");
        return *cur++;
    }
    void skip_ws() noexcept {
        while (cur < end && (*cur == ' ' || *cur == '\t' || *cur == '\n' || *cur == '\r'))
            ++cur;
    }
    void require(char c) {
        skip_ws();
        if (peek() != c)
            throw std::runtime_error(std::string{"json::parse: expected '"} + c + '\'');
        ++cur;
    }

    value parse_val();
    value parse_obj();
    value parse_arr();
    value parse_str();
    value parse_lit();
    value parse_num();
};

inline value parser::parse_val() {
    skip_ws();
    switch (peek()) {
        case '{': return parse_obj();
        case '[': return parse_arr();
        case '"': return parse_str();
        case 't': case 'f': case 'n': return parse_lit();
        case '-':
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            return parse_num();
        default: break;
    }
    throw std::runtime_error(std::string{"json::parse: unexpected '"} + peek() + '\'');
}

inline value parser::parse_obj() {
    require('{');
    object_t obj;
    skip_ws();
    if (peek() == '}') { ++cur; return value{std::move(obj)}; }
    for (;;) {
        skip_ws();
        if (peek() != '"') throw std::runtime_error("json::parse: expected string key");
        std::string key = parse_str().as<string_t>();
        require(':');
        obj[std::move(key)] = parse_val();
        skip_ws();
        if (peek() == '}') { ++cur; break; }
        require(',');
    }
    return value{std::move(obj)};
}

inline value parser::parse_arr() {
    require('[');
    array_t arr;
    skip_ws();
    if (peek() == ']') { ++cur; return value{std::move(arr)}; }
    for (;;) {
        arr.push_back(parse_val());
        skip_ws();
        if (peek() == ']') { ++cur; break; }
        require(',');
    }
    return value{std::move(arr)};
}

inline value parser::parse_str() {
    require('"');
    std::string s;
    for (;;) {
        if (cur >= end) throw std::runtime_error("json::parse: unterminated string");
        char c = *cur++;
        if (c == '"') break;
        if (c != '\\') { s += c; continue; }
        if (cur >= end) throw std::runtime_error("json::parse: bare backslash at end");
        char esc = *cur++;
        switch (esc) {
            case '"':  s += '"';  break;
            case '\\': s += '\\'; break;
            case '/':  s += '/';  break;
            case 'b':  s += '\b'; break;
            case 'f':  s += '\f'; break;
            case 'n':  s += '\n'; break;
            case 'r':  s += '\r'; break;
            case 't':  s += '\t'; break;
            case 'u': {
                if (end - cur < 4)
                    throw std::runtime_error("json::parse: truncated \\u escape");
                unsigned cp = 0;
                for (int i = 0; i < 4; ++i) {
                    unsigned char h = static_cast<unsigned char>(*cur++);
                    cp <<= 4;
                    if (h >= '0' && h <= '9')      cp |= h - '0';
                    else if (h >= 'a' && h <= 'f') cp |= h - 'a' + 10;
                    else if (h >= 'A' && h <= 'F') cp |= h - 'A' + 10;
                    else throw std::runtime_error("json::parse: bad hex in \\u");
                }
                // Encode as UTF-8
                if (cp < 0x80) {
                    s += static_cast<char>(cp);
                } else if (cp < 0x800) {
                    s += static_cast<char>(0xC0 | (cp >> 6));
                    s += static_cast<char>(0x80 | (cp & 0x3F));
                } else {
                    s += static_cast<char>(0xE0 | (cp >> 12));
                    s += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                    s += static_cast<char>(0x80 | (cp & 0x3F));
                }
                break;
            }
            default:
                throw std::runtime_error(std::string{"json::parse: bad escape \\"} + esc);
        }
    }
    return value{std::move(s)};
}

inline value parser::parse_lit() {
    auto match = [&](const char* lit, std::size_t n) -> bool {
        if (static_cast<std::size_t>(end - cur) < n) return false;
        if (std::memcmp(cur, lit, n) != 0) return false;
        cur += n;
        return true;
    };
    if (match("true",  4)) return value{true};
    if (match("false", 5)) return value{false};
    if (match("null",  4)) return value{};
    throw std::runtime_error("json::parse: bad literal");
}

inline value parser::parse_num() {
    const char* start = cur;
    if (peek() == '-') ++cur;

    if (cur < end && *cur == '0') {
        ++cur;
    } else {
        if (cur >= end || *cur < '1' || *cur > '9')
            throw std::runtime_error("json::parse: bad number");
        while (cur < end && *cur >= '0' && *cur <= '9') ++cur;
    }

    bool fp = false;
    if (cur < end && *cur == '.') {
        fp = true; ++cur;
        if (cur >= end || *cur < '0' || *cur > '9')
            throw std::runtime_error("json::parse: expected digit after '.'");
        while (cur < end && *cur >= '0' && *cur <= '9') ++cur;
    }
    if (cur < end && (*cur == 'e' || *cur == 'E')) {
        fp = true; ++cur;
        if (cur < end && (*cur == '+' || *cur == '-')) ++cur;
        if (cur >= end || *cur < '0' || *cur > '9')
            throw std::runtime_error("json::parse: expected digit in exponent");
        while (cur < end && *cur >= '0' && *cur <= '9') ++cur;
    }

    if (fp) {
        double d{};
        auto [p, ec] = std::from_chars(start, cur, d);
        if (ec != std::errc{}) throw std::runtime_error("json::parse: bad float");
        return value{d};
    } else {
        std::int64_t i{};
        auto [p, ec] = std::from_chars(start, cur, i);
        if (ec != std::errc{}) throw std::runtime_error("json::parse: bad integer");
        return value{i};
    }
}

} // namespace detail

inline value parse(std::string_view src) {
    detail::parser p{src.data(), src.data() + src.size()};
    value v = p.parse_val();
    p.skip_ws();
    if (p.cur != p.end)
        throw std::runtime_error("json::parse: trailing content after value");
    return v;
}


// Top-level convenience API
// Serialize any C++ type to a JSON string.
// indent = 0  →  compact  (default)
// indent > 0  →  pretty-print with that many spaces per level
template <typename T>
std::string serialize(const T& t, int indent = 0) {
    return stringify(to_value(t), indent);
}

// Deserialize a JSON string directly into a C++ type.
template <typename T>
T deserialize(std::string_view src) {
    return from_value<T>(parse(src));
}

} // namespace json


// Usage example (compiled out unless you #define JSON26_EXAMPLE)
#ifdef JSON26_EXAMPLE
#include <cassert>
#include <iostream>
#include <print>

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

    // Serialize
    std::string json = json::serialize(alice, /*indent=*/2);
    std::println("{}", json);

    // Round-trip
    Person bob = json::deserialize<Person>(json);
    assert(bob.name  == alice.name);
    assert(bob.age   == alice.age);
    assert(bob.score == alice.score);
    assert(bob.favorite_color == Color::Blue);
    assert(bob.address->city  == "Wonderland");

    // Direct value manipulation
    json::value v = json::parse(R"({"x":1,"y":[2,3]})");
    v["z"] = json::value{true};
    std::println("{}", json::stringify(v));

    // std::map round-trip
    std::map<std::string, int> scores{{"a", 1}, {"b", 2}};
    auto scores_json = json::serialize(scores);
    auto scores2     = json::deserialize<std::map<std::string, int>>(scores_json);
    assert(scores == scores2);
}
#endif // JSON26_EXAMPLE
