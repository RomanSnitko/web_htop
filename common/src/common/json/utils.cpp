/**
 * @file common/json/utils.cpp
 * 
 * @author Maksim Vashkevich
 * @date 2026-04-06
 * 
 * @brief Implementation of `JSONValue` operations and serialization.
 * @details Provides type queries, typed accessors, indexed access helpers and
 * conversion of internal JSON representation to compact or pretty-printed JSON.
 */

#include "common/json/utils.hpp"

#include <algorithm>
#include <cstdio>
#include <string>
#include <limits>
#include <memory>

namespace web_htop::json::utils {

JSONValue::JSONValue(std::monostate ms)   : value_(ms) {}
JSONValue::JSONValue(bool b)              : value_(b) {}
JSONValue::JSONValue(std::int64_t i64)    : value_(i64) {}
JSONValue::JSONValue(std::uint64_t ui64)  : value_(ui64) {}
JSONValue::JSONValue(double d)            : value_(d) {}
JSONValue::JSONValue(std::string_view sw) : value_(sw) {}
JSONValue::JSONValue(char const * s)      : value_(std::string_view(s)) {}
JSONValue::JSONValue(JSONArray && arr)    : value_(std::make_unique<JSONArray>(std::move(arr))) {}
JSONValue::JSONValue(JSONObject && obj)   : value_(std::make_unique<JSONObject>(std::move(obj))) {}

bool JSONValue::IsNull() const {
	return std::holds_alternative<std::monostate>(value_);
}

bool JSONValue::IsBool() const {
	return std::holds_alternative<bool>(value_);
}

bool JSONValue::IsInteger() const {
	return (
		std::holds_alternative<std::int64_t>(value_) ||
		std::holds_alternative<std::uint64_t>(value_)
	);
}

bool JSONValue::IsFloatingPoint() const {
	return std::holds_alternative<double>(value_);
}

bool JSONValue::IsString() const {
	return std::holds_alternative<std::string_view>(value_);
}

bool JSONValue::IsArray() const {
	return std::holds_alternative<std::unique_ptr<JSONArray>>(value_);
}

bool JSONValue::IsObject() const {
	return std::holds_alternative<std::unique_ptr<JSONObject>>(value_);
}

std::optional<bool> JSONValue::AsBool() const {
	if (auto * v = std::get_if<bool>(&value_)) {
		return *v;
	}
	
	return std::nullopt;
}

std::optional<std::int64_t> JSONValue::AsInt64() const {
	if (auto * v = std::get_if<std::int64_t>(&value_)) {
		return *v;
	}
	
	if (auto * v = std::get_if<std::uint64_t>(&value_)) {
		if (*v <= static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
			return static_cast<std::int64_t>(*v);
		}
	}
	
	return std::nullopt;
}

std::optional<std::uint64_t> JSONValue::AsUInt64() const {
	if (auto * v = std::get_if<std::uint64_t>(&value_)) {
		return *v;
	}
	
	if (auto * v = std::get_if<std::int64_t>(&value_)) {
		if (*v >= 0) {
			return static_cast<std::uint64_t>(*v);
		}
	}
	
	return std::nullopt;
}

std::optional<double> JSONValue::AsDouble() const {
	if (auto * v = std::get_if<double>(&value_)) {
		return *v;
	}
	
	if (auto * v = std::get_if<std::int64_t>(&value_)) {
		return static_cast<double>(*v);
	}
	
	if (auto * v = std::get_if<std::uint64_t>(&value_)) {
		return static_cast<double>(*v);
	}
	
	return std::nullopt;
}

std::optional<std::string_view> JSONValue::AsString() const {
	if (auto * v = std::get_if<std::string_view>(&value_)) {
		return *v;
	}
	
	return std::nullopt;
}

JSONArray const * JSONValue::AsArray() const {
	if (auto * v = std::get_if<std::unique_ptr<JSONArray>>(&value_)) {
		return v->get();
	}
	
	return nullptr;
}

JSONObject const * JSONValue::AsObject() const {
	if (auto * v = std::get_if<std::unique_ptr<JSONObject>>(&value_)) {
		return v->get();
	}
	
	return nullptr;
}

JSONArray * JSONValue::AsArray() {
	return const_cast<JSONArray *>(static_cast<JSONValue const *>(this)->AsArray());
}

JSONObject * JSONValue::AsObject() {
	return const_cast<JSONObject *>(static_cast<JSONValue const *>(this)->AsObject());
}

std::optional<std::reference_wrapper<JSONValue const>> JSONValue::operator[](std::string_view key) const {
	if (auto * obj = AsObject()) {
		auto it = std::find_if(obj->begin(), obj->end(), [&key](auto const & pair) {
			return pair.first == key;
		});
		
		if (it != obj->end()) {
			return std::cref(it->second);
		}
	}
	
	return std::nullopt;
}

std::optional<std::reference_wrapper<JSONValue const>> JSONValue::operator[](size_t index) const {
	if (auto * arr = AsArray()) {
		if (index < arr->size()) {
			return std::cref((*arr)[index]);
		}
	}
	
	return std::nullopt;
}

std::optional<std::reference_wrapper<JSONValue>> JSONValue::operator[](std::string_view key) {
	if (auto val = static_cast<JSONValue const &>(*this)[key]) {
		return std::ref(const_cast<JSONValue &>(val->get()));
	}
	
	return std::nullopt;
}

std::optional<std::reference_wrapper<JSONValue>> JSONValue::operator[](size_t index) {
	if (auto val = static_cast<JSONValue const &>(*this)[index]) {
		return std::ref(const_cast<JSONValue &>(val->get()));
	}
	
	return std::nullopt;
}

bool JSONValue::HasField(std::string_view key) const {
	if (auto * obj = AsObject()) {
		return std::any_of(obj->begin(), obj->end(), [key](auto const & pair) {
			return pair.first == key;
		});
	}
	
	return false;
}

size_t JSONValue::size() const {
	if (auto * arr = AsArray()) {
		return arr->size();
	}
	
	if (auto * obj = AsObject()) {
		return obj->size();
	}
	
	return 0;
	
}

namespace {

/**
 * @brief Append JSON-escaped string literal to output.
 * @details Escapes control characters and JSON metacharacters according to JSON
 * string encoding rules.
 * @param out Destination string buffer.
 * @param sw Source string view to escape and quote.
 * @returns Nothing.
 */
void AppendEscapedString_(std::string & out, std::string_view sw) {
	out += '"';
	for (unsigned char c : sw) {
		switch (c) {
			case '"':
				out += "\\\"";
				break;
			case '\\':
				out += "\\\\";
				break;
			case '\b':
				out += "\\b";
				break;
			case '\f':
				out += "\\f";
				break;
			case '\n':
				out += "\\n";
				break;
			case '\r':
				out += "\\r";
				break;
			case '\t':
				out += "\\t";
				break;
			default:
				if (c < 0x20) {
					char buf[7];
					snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned>(c));
					
					out += buf;
				} else {
					out += static_cast<char>(c);
				}
		}
	}
	out += '"';
}

/**
 * @brief Append indentation for pretty JSON output.
 * @details Adds newline and indentation spaces when pretty mode is enabled.
 * @param out Destination string buffer.
 * @param indent Number of spaces per nesting level.
 * @param level Current nesting level.
 * @returns Nothing.
 */
void AppendIndent_(std::string & out, int32_t indent, int32_t level) {
	if (indent >= 0) {
		out += '\n';
		out.append(static_cast<size_t>(indent) * level, ' ');
	}
}

} // namespace

/**
 * @brief Recursively serialize JSON value to output string.
 * @details Visits active variant alternative and serializes nested arrays and
 * objects with optional pretty indentation.
 * @param out Destination string buffer.
 * @param indent Number of spaces per nesting level; negative for compact mode.
 * @param currentLevel Current nesting depth.
 * @returns Nothing.
 */
void JSONValue::SerializeTo_(std::string & out, int32_t indent, int32_t currentLevel) const {
	std::visit([&](auto const & val) {
		using T = std::decay_t<decltype(val)>;
		
		if constexpr (std::is_same_v<T, std::monostate>) {
			out += "null";
		} else if constexpr (std::is_same_v<T, bool>) {
			out += val ? "true" : "false";
		} else if constexpr (std::is_same_v<T, std::int64_t> ||
		                     std::is_same_v<T, std::uint64_t>) {
			out += std::to_string(val);
		} else if constexpr (std::is_same_v<T, double>) {
			char buf[64];
			int len = snprintf(buf, sizeof(buf), "%.17g", val);
			if (len > 0) {
				std::string s(buf, len);
				std::replace(s.begin(), s.end(), ',', '.');
				
				out += s;
			}
		} else if constexpr (std::is_same_v<T, std::string_view>) {
			AppendEscapedString_(out, val);
		} else if constexpr (std::is_same_v<T, std::unique_ptr<JSONArray>>) {
			if (!val || val->empty()) {
				out += "[]";
				return;
			}
			
			out += '[';
			for (size_t i = 0; i < val->size(); i++) {
				AppendIndent_(out, indent, currentLevel + 1);
				
				(*val)[i].SerializeTo_(out, indent, currentLevel + 1);
				
				if (i + 1 < val->size()) {
					out += ',';
				}
			}
			AppendIndent_(out, indent, currentLevel);
			out += ']';
		} else if constexpr (std::is_same_v<T, std::unique_ptr<JSONObject>>) {
			if (!val || val->empty()) {
				out += "{}";
				return;
			}
			
			out += '{';
			for (size_t i = 0; i < val->size(); i++) {
				AppendIndent_(out, indent, currentLevel + 1);
				AppendEscapedString_(out, (*val)[i].first);
				
				out += (indent >= 0 ? ": " : ":");
				(*val)[i].second.SerializeTo_(out, indent, currentLevel + 1);
				
				if (i + 1 < val->size()) {
					out += ',';
				}
			}
			AppendIndent_(out, indent, currentLevel);
			out += '}';
		}
	}, value_);
}

std::string JSONValue::ToString(int32_t indent) const {
	std::string serialized;
	serialized.reserve(IsArray() ? (size() * 128) : 512);
	
	SerializeTo_(serialized, indent, 0);
	
	return serialized;
}

} // namespace web_htop::json::utils