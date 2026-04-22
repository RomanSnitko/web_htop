/**
 * @file common/json/utils.hpp
 * 
 * @author Maksim Vashkevich
 * @date 2026-04-05
 * 
 * @brief Lightweight JSON value model for web_htop.
 * @details This header defines `JSONValue` and related aliases used by parser
 * and model serialization logic without external JSON dependencies.
 */

#ifndef WEB_HTOP_COMMON_JSON_UTILS_HPP_
#define WEB_HTOP_COMMON_JSON_UTILS_HPP_

#include <cstddef>     // std::size_t
#include <cstdint>     // std::int64_t, std::uint64_t
#include <functional>  // std::reference_wrapper
#include <memory>      // std::unique_ptr
#include <optional>    // std::optional
#include <string_view> // std::string_view
#include <string>      // std::string
#include <variant>     // std::variant
#include <vector>      // std::vector
#include <utility>     // std::pair

namespace web_htop::json::utils {

class JSONValue;

/// @brief JSON object representation.
using JSONObject = std::vector<std::pair<std::string_view, JSONValue>>;

using JSONArray = std::vector<JSONValue>; ///< JSON array representation

/**
 * @brief Underlying variant for all supported JSON types.
 * @details Containers are heap-allocated to keep `JSONValue` movable even for
 * recursive array/object structures.
 */
using JSONVariant = std::variant<
    std::monostate,
    bool,
    std::int64_t,
    std::uint64_t,
    double,
    std::string_view,
    std::unique_ptr<JSONArray>,
    std::unique_ptr<JSONObject>
>;

/**
 * @brief Variant-based JSON value container.
 * @details Stores one JSON type at a time and provides type checks, typed
 * accessors, indexing and string serialization.
 */
class JSONValue {
public:
    /**
     * @brief Construct null JSON value.
     * @details Initializes value as JSON `null`.
     * @param ms Marker value for null state.
     * @returns Constructed null `JSONValue`.
     */
    explicit JSONValue(std::monostate ms = std::monostate{});

    /**
     * @brief Construct boolean JSON value.
     * @details Stores JSON `true` or `false`.
     * @param b Boolean value to store.
     * @returns Constructed boolean `JSONValue`.
     */
    explicit JSONValue(bool b);

    /**
     * @brief Construct signed integer JSON value.
     * @details Stores a 64-bit signed integer number.
     * @param i64 Signed integer value.
     * @returns Constructed integer `JSONValue`.
     */
    explicit JSONValue(std::int64_t i64);

    /**
     * @brief Construct unsigned integer JSON value.
     * @details Stores a 64-bit unsigned integer number.
     * @param ui64 Unsigned integer value.
     * @returns Constructed integer `JSONValue`.
     */
    explicit JSONValue(std::uint64_t ui64);

    /**
     * @brief Construct floating-point JSON value.
     * @details Stores a double-precision JSON number.
     * @param d Floating-point value.
     * @returns Constructed floating-point `JSONValue`.
     */
    explicit JSONValue(double d);

    /**
     * @brief Construct string JSON value from view.
     * @details Stores non-owning string view; caller must guarantee lifetime.
     * @param sw String view to store.
     * @returns Constructed string `JSONValue`.
     */
    explicit JSONValue(std::string_view sw);

    /**
     * @brief Construct string JSON value from C string.
     * @details Stores `std::string_view` over provided null-terminated string.
     * @param s Pointer to null-terminated UTF-8 string.
     * @returns Constructed string `JSONValue`.
     */
    explicit JSONValue(char const * s);

    /**
     * @brief Construct array JSON value.
     * @details Takes ownership of array contents via move.
     * @param arr Array value to store.
     * @returns Constructed array `JSONValue`.
     */
    explicit JSONValue(JSONArray && arr);

    /**
     * @brief Construct object JSON value.
     * @details Takes ownership of object key-value pairs via move.
     * @param obj Object value to store.
     * @returns Constructed object `JSONValue`.
     */
    explicit JSONValue(JSONObject && obj);

    JSONValue(JSONValue const &) = delete;
    JSONValue & operator=(JSONValue const &) = delete;

    JSONValue(JSONValue &&) noexcept = default;
    JSONValue & operator=(JSONValue &&) noexcept = default;

    /**
     * @brief Check if value is JSON null.
     * @details Verifies variant currently stores `std::monostate`.
     * @returns `true` when value is null, otherwise `false`.
     */
    [[nodiscard]] bool IsNull() const;

    /**
     * @brief Check if value is boolean.
     * @details Verifies variant currently stores `bool`.
     * @returns `true` for boolean values, otherwise `false`.
     */
    [[nodiscard]] bool IsBool() const;

    /**
     * @brief Check if value is integer.
     * @details Returns `true` for both signed and unsigned 64-bit integers.
     * @returns `true` when value is integer, otherwise `false`.
     */
    [[nodiscard]] bool IsInteger() const;

    /**
     * @brief Check if value is floating-point number.
     * @details Verifies variant currently stores `double`.
     * @returns `true` for floating-point values, otherwise `false`.
     */
    [[nodiscard]] bool IsFloatingPoint() const;

    /**
     * @brief Check if value is string.
     * @details Verifies variant currently stores `std::string_view`.
     * @returns `true` for string values, otherwise `false`.
     */
    [[nodiscard]] bool IsString() const;

    /**
     * @brief Check if value is array.
     * @details Verifies variant currently stores array container.
     * @returns `true` for array values, otherwise `false`.
     */
    [[nodiscard]] bool IsArray() const;

    /**
     * @brief Check if value is object.
     * @details Verifies variant currently stores object container.
     * @returns `true` for object values, otherwise `false`.
     */
    [[nodiscard]] bool IsObject() const;

    /**
     * @brief Get value as boolean.
     * @details Returns present only if current type is `bool`.
     * @returns Optional boolean value.
     */
    [[nodiscard]] std::optional<bool> AsBool() const;

    /**
     * @brief Get value as signed integer.
     * @details Converts from unsigned integer only if value fits `int64_t`.
     * @returns Optional signed integer value.
     */
    [[nodiscard]] std::optional<std::int64_t> AsInt64() const;

    /**
     * @brief Get value as unsigned integer.
     * @details Converts from signed integer only for non-negative values.
     * @returns Optional unsigned integer value.
     */
    [[nodiscard]] std::optional<std::uint64_t> AsUInt64() const;

    /**
     * @brief Get value as floating-point number.
     * @details Converts integer types to `double` when needed.
     * @returns Optional floating-point value.
     */
    [[nodiscard]] std::optional<double> AsDouble() const;

    /**
     * @brief Get value as string.
     * @details Returns underlying non-owning `std::string_view`.
     * @returns Optional string view.
     */
    [[nodiscard]] std::optional<std::string_view> AsString() const;

    /**
     * @brief Access value as const array pointer.
     * @details Returns pointer only when value stores an array.
     * @returns Pointer to array or `nullptr`.
     */
    [[nodiscard]] JSONArray const * AsArray() const;

    /**
     * @brief Access value as const object pointer.
     * @details Returns pointer only when value stores an object.
     * @returns Pointer to object or `nullptr`.
     */
    [[nodiscard]] JSONObject const * AsObject() const;

    /**
     * @brief Access value as mutable array pointer.
     * @details Provides mutable access when current value is array.
     * @returns Pointer to array or `nullptr`.
     */
    [[nodiscard]] JSONArray * AsArray();

    /**
     * @brief Access value as mutable object pointer.
     * @details Provides mutable access when current value is object.
     * @returns Pointer to object or `nullptr`.
     */
    [[nodiscard]] JSONObject * AsObject();

    /**
     * @brief Access object field by key (const).
     * @details Looks up key in object preserving insertion order.
     * @param key Object field name.
     * @returns Optional reference to field value.
     */
    [[nodiscard]] std::optional<std::reference_wrapper<JSONValue const>> operator[](std::string_view key) const;

    /**
     * @brief Access array element by index (const).
     * @details Returns element only if current value is array and index is valid.
     * @param index Zero-based array index.
     * @returns Optional reference to element value.
     */
    [[nodiscard]] std::optional<std::reference_wrapper<JSONValue const>> operator[](size_t index) const;

    /**
     * @brief Access object field by key (mutable).
     * @details Looks up key in object and returns mutable reference.
     * @param key Object field name.
     * @returns Optional mutable reference to field value.
     */
    [[nodiscard]] std::optional<std::reference_wrapper<JSONValue>> operator[](std::string_view key);

    /**
     * @brief Access array element by index (mutable).
     * @details Returns mutable element reference for valid array index.
     * @param index Zero-based array index.
     * @returns Optional mutable reference to element value.
     */
    [[nodiscard]] std::optional<std::reference_wrapper<JSONValue>> operator[](size_t index);

    /**
     * @brief Check whether object has specified field.
     * @details Performs linear lookup over object key-value pairs.
     * @param key Object field name to search for.
     * @returns `true` when field exists, otherwise `false`.
     */
    [[nodiscard]] bool HasField(std::string_view key) const;

    /**
     * @brief Get container size.
     * @details Returns number of elements for arrays/objects, `0` otherwise.
     * @returns Element count for container values.
     */
    [[nodiscard]] size_t size() const;

    /**
     * @brief Serialize value into JSON string.
     * @details Produces compact JSON by default or pretty output when
     * `indent >= 0`.
     * @param indent Number of spaces per nesting level; `-1` for compact mode.
     * @returns Serialized JSON text.
     */
    [[nodiscard]] std::string ToString(int32_t indent = -1) const;

private:
    JSONVariant value_;

    /**
     * @brief Internal recursive serializer.
     * @details Appends current value to output string with selected indentation.
     * @param out Output JSON string buffer.
     * @param indent Number of spaces per indentation level.
     * @param currentLevel Current recursion depth.
     * @returns None.
     */
    void SerializeTo_(std::string & out, int32_t indent, int32_t currentLevel) const;
};

} // namespace web_htop::json::utils

#endif // WEB_HTOP_COMMON_JSON_UTILS_HPP_