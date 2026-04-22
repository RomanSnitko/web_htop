/**
 * @file common/json/parser.cpp
 * 
 * @author Maksim Vashkevich
 * @date 2026-04-06
 * 
 * @brief JSON parser implementation for lightweight internal JSON model.
 * @details Contains parsing logic for all JSON value kinds,
 * Unicode escape decoding, and construction of `utils::JSONValue` trees with
 * stable string storage.
 */

#include "common/json/parser.hpp"

#include <charconv>
#include <cctype>
#include <cstdlib>
#include <string>

namespace web_htop::json {

namespace {

/**
 * @brief Convert one hexadecimal digit into numeric value.
 * @details Supports decimal and Latin hexadecimal digits in both cases.
 * @param c Hexadecimal character to decode.
 * @returns Numeric value in range [0..15] or `std::nullopt` if input is invalid.
 */
[[nodiscard]] std::optional<uint32_t> HexDigit_(char c) {
	if (c >= '0' && c <= '9') {
		return static_cast<uint32_t>(c - '0');
	}
	if (c >= 'a' && c <= 'f') {
		return static_cast<uint32_t>(c - 'a' + 10);
	}
	if (c >= 'A' && c <= 'F') {
		return static_cast<uint32_t>(c - 'A' + 10);
	}
	
	return std::nullopt;
}

/**
 * @brief Parse four hexadecimal digits from string segment.
 * @details Parses exactly four characters starting at index \p i for JSON
 * `\uXXXX` escape decoding.
 * @param raw Source string containing hexadecimal digits.
 * @param i Start position of the 4-digit sequence.
 * @returns Parsed 16-bit code unit value or `std::nullopt` if input is invalid.
 */
[[nodiscard]] std::optional<uint32_t> ParseHex4_(std::string_view raw, size_t i) {
	if (i + 4 > raw.size()) {
		return std::nullopt;
	}
	
	uint32_t res = 0;
	for (size_t k = 0; k < 4; k++) {
		auto digit = HexDigit_(raw[i + k]);
		if (!digit) {
			return std::nullopt;
		}
		
		res = (res << 4) | *digit;
	}
	
	return res;
}

/**
 * @brief Append one Unicode code point encoded as UTF-8.
 * @details Encodes code point into 1-4 UTF-8 bytes and appends to \p out.
 * @param out Target string for encoded bytes. Modified in place.
 * @param cp Unicode code point to encode.
 * @returns `true` when code point is valid and encoded, otherwise `false`.
 */
[[nodiscard]] bool EncodeUtf8_(std::string &out, uint32_t cp) {
	if (cp < 0x80u) {
		out += static_cast<char>(cp);
	} else if (cp < 0x800u) {
		out += static_cast<char>(0xC0u | (cp >> 6));
		out += static_cast<char>(0x80u | (cp & 0x3Fu));
	} else if (cp < 0x10000u) {
		out += static_cast<char>(0xE0u | (cp >> 12));
		out += static_cast<char>(0x80u | ((cp >> 6) & 0x3Fu));
		out += static_cast<char>(0x80u | (cp & 0x3Fu));
	} else if (cp <= 0x10FFFFu) {
		out += static_cast<char>(0xF0u | (cp >> 18));
		out += static_cast<char>(0x80u | ((cp >> 12) & 0x3Fu));
		out += static_cast<char>(0x80u | ((cp >> 6) & 0x3Fu));
		out += static_cast<char>(0x80u | (cp & 0x3Fu));
	} else {
		return false;
	}

	return true;
}

/**
 * @brief Mutable parser state for one JSON input.
 * @details Tracks current cursor position and owns destination storage for
 * decoded strings referenced by parsed `JSONValue` instances.
 */
struct ParserState {
	std::string_view input; ///< Input string to parse
	size_t pos = 0;         ///< Current position in the input string
	StringStorage &storage; ///< Owning container for decoded strings
	
	/**
	 * @brief Check whether parser cursor is at input end.
	 * @details Uses current \p pos against `input.size()`.
	 * @returns `true` if no more input is available, otherwise `false`.
	 */
	[[nodiscard]] bool AtEnd() const noexcept {
		return pos >= input.size();
	}
	
	/**
	 * @brief Read current character without consuming it.
	 * @details Returns `'\0'` when parser already reached end of input.
	 * @returns Current input character or null character at end.
	 */
	[[nodiscard]] char Peek() const noexcept {
		return AtEnd() ? '\0' : input[pos];
	}
	
	/**
	 * @brief Consume expected character from input.
	 * @details Advances parser by one symbol only if next character matches.
	 * @param c Expected character.
	 * @returns `true` when expected character is consumed, otherwise `false`.
	 */
	bool Expect(char c) {
		if (AtEnd() || input[pos] != c) {
			return false;
		}
		
		++pos;
		
		return true;
	}
	
	/**
	 * @brief Skip JSON whitespace characters.
	 * @details Consumes spaces, tabs, newlines and carriage returns.
	 * @returns None.
	 */
	void SkipWhitespace() noexcept {
		while (pos < input.size()) {
			char const c = input[pos];
			
			if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
				++pos;
			} else {
				break;
			}
		}
	}
	
	/**
	 * @brief Parse JSON string and decode escape sequences.
	 * @details Parses quoted JSON string, validates escapes including surrogate
	 * pairs, and stores content in storage.
	 * @returns String view into stable owned storage on success; `std::nullopt`
	 * when string literal is malformed.
	 */
	[[nodiscard]] std::optional<std::string_view> ParseString() {
		if (!Expect('"')) {
			return std::nullopt;
		}
		
		size_t const start = pos;
		bool needs_decode = false;
		
		while (!AtEnd()) {
			char const c = input[pos];
			if (c == '"') {
				break;
			}
			
			if (c == '\\') {
				needs_decode = true;
				++pos;
				
				if (AtEnd()) {
					return std::nullopt;
				}
				
				char esc = input[pos++];
				if (esc == 'u') {
					if (pos + 4 > input.size()) {
						return std::nullopt;
					}
					
					for (int i = 0; i < 4; i++) {
						if (!HexDigit_(input[pos + i])) {
							return std::nullopt;
						}
					}
					
					pos += 4;
				}
				
				continue;
			}
			
			if (static_cast<unsigned char>(c) < 0x20) {
				return std::nullopt;
			}
			
			++pos;
		}
		
		if (!Expect('"')) {
			return std::nullopt;
		}
		
		std::string_view const raw = input.substr(start, pos - start - 1);
		if (!needs_decode) {
			storage.emplace_back(raw);
			return storage.back();
		}
		
		std::string decoded;
		decoded.reserve(raw.size());
		
		for (size_t i = 0; i < raw.size();) {
			if (raw[i] != '\\') {
				decoded += raw[i++];
				continue;
			}
			
			i++;
			char esc = raw[i++];
			switch (esc) {
				case '"':
					decoded += '"';
					break;
				case '\\':
					decoded += '\\';
					break;
				case '/':
					decoded += '/';
					break;
				case 'b':
					decoded += '\b';
					break;
				case 'f':
					decoded += '\f';
					break;
				case 'n':
					decoded += '\n';
					break;
				case 'r':
					decoded += '\r';
					break;
				case 't':
					decoded += '\t';
					break;
				case 'u': {
					auto cp_opt = ParseHex4_(raw, i);
					if (!cp_opt) {
						return std::nullopt;
					}
					
					uint32_t cp = *cp_opt;
					i += 4;
					
					if (cp >= 0xD800u && cp <= 0xDBFFu) {
						if (i + 6 <= raw.size() && raw[i] == '\\' && raw[i + 1] == 'u') {
							auto low_opt = ParseHex4_(raw, i + 2);
							if (!low_opt || *low_opt < 0xDC00u || *low_opt > 0xDFFFu) {
								return std::nullopt;
							}
							
							cp = 0x10000u + ((cp - 0xD800u) << 10) + (*low_opt - 0xDC00u);
							i += 6;
						} else {
							return std::nullopt;
						}
					} else if (cp >= 0xDC00u && cp <= 0xDFFFu) {
						return std::nullopt;
					}
					
					if (!EncodeUtf8_(decoded, cp)) {
						return std::nullopt;
					}
					break;
				}
				default:
					return std::nullopt;
			}
		}
		
		storage.push_back(std::move(decoded));
		return storage.back();
	}
	
	/**
	 * @brief Parse JSON number token.
	 * @details Supports integer and floating-point forms with optional sign and
	 * exponent, validating syntax before conversion.
	 * @returns Parsed numeric JSON value or `std::nullopt` if input is malformed.
	 */
	[[nodiscard]] std::optional<utils::JSONValue> ParseNumber() {
		size_t const start = pos;
		if (Peek() == '-') {
			++pos;
		}
		
		if (!std::isdigit(static_cast<unsigned char>(Peek()))) {
			return std::nullopt;
		}
		
		if (Peek() == '0') {
			++pos;
			
			if (!AtEnd() && std::isdigit(static_cast<unsigned char>(Peek()))) {
				return std::nullopt;
			}
		} else {
			while (!AtEnd() && std::isdigit(static_cast<unsigned char>(input[pos]))) {
				++pos;
			}
		}
		
		bool is_float = false;
		if (!AtEnd() && input[pos] == '.') {
			is_float = true;
			++pos;
			
			if (AtEnd() || !std::isdigit(static_cast<unsigned char>(input[pos]))) {
				return std::nullopt;
			}
			while (!AtEnd() && std::isdigit(static_cast<unsigned char>(input[pos]))) {
				++pos;
			}
		}
		
		if (!AtEnd() && (input[pos] == 'e' || input[pos] == 'E')) {
			is_float = true;
			++pos;
			
			if (!AtEnd() && (input[pos] == '+' || input[pos] == '-')) {
				++pos;
			}
			if (AtEnd() || !std::isdigit(static_cast<unsigned char>(input[pos]))) {
				return std::nullopt;
			}
			while (!AtEnd() && std::isdigit(static_cast<unsigned char>(input[pos]))) {
				++pos;
			}
		}
		
		std::string_view const num_str = input.substr(start, pos - start);
		
		if (is_float) {
			std::string const num_copy(num_str);
			char * end_ptr = nullptr;
			double const d = std::strtod(num_copy.c_str(), &end_ptr);
			
			if (end_ptr != num_copy.c_str() + num_copy.size()) {
				return std::nullopt;
			}
			return utils::JSONValue(d);
		}
		
		if (num_str[0] == '-') {
			int64_t i64;
			auto [p, ec] = std::from_chars(num_str.data(), num_str.data() + num_str.size(), i64);
			if (ec != std::errc{}) {
				return std::nullopt;
			}
			
			return utils::JSONValue(i64);
		}
		
		uint64_t u64;
		auto [p, ec] = std::from_chars(num_str.data(), num_str.data() + num_str.size(), u64);
		if (ec != std::errc{}) {
			return std::nullopt;
		}
		
		return utils::JSONValue(u64);
	}
	
	/**
	 * @brief Parse JSON array value.
	 * @details Parses comma-separated list of nested JSON values enclosed in
	 * square brackets.
	 * @returns Parsed array value or `std::nullopt` if input is malformed.
	 */
	[[nodiscard]] std::optional<utils::JSONValue> ParseArray() {
		if (!Expect('[')) {
			return std::nullopt;
		}
		SkipWhitespace();
		
		utils::JSONArray arr;
		if (Peek() == ']') {
			++pos;
			return utils::JSONValue(std::move(arr));
		}
		
		while (true) {
			auto val = ParseValue();
			if (!val) {
				return std::nullopt;
			}
			
			arr.push_back(std::move(*val));
			
			SkipWhitespace();
			char c = Peek();
			if (c == ']') {
				++pos;
				break;
			}
			if (c == ',') {
				++pos;
				SkipWhitespace();
				if (Peek() == ']') {
					return std::nullopt;
				}
			} else {
				return std::nullopt;
			}
		}
		
		return utils::JSONValue(std::move(arr));
	}
	
	/**
	 * @brief Parse JSON object value.
	 * @details Parses string-key/value pairs separated by commas and enclosed in
	 * curly braces.
	 * @returns Parsed object value or `std::nullopt` if input is malformed.
	 */
	[[nodiscard]] std::optional<utils::JSONValue> ParseObject() {
		if (!Expect('{')) {
			return std::nullopt;
		}
		SkipWhitespace();
		
		utils::JSONObject obj;
		if (Peek() == '}') {
			++pos;
			return utils::JSONValue(std::move(obj));
		}
		
		while (true) {
			SkipWhitespace();
			
			auto key = ParseString();
			if (!key) {
				return std::nullopt;
			}
			
			SkipWhitespace();
			if (!Expect(':')) {
				return std::nullopt;
			}
			SkipWhitespace();
			
			auto val = ParseValue();
			if (!val) {
				return std::nullopt;
			}
			
			obj.emplace_back(*key, std::move(*val));
			SkipWhitespace();
			
			char c = Peek();
			if (c == '}') {
				++pos;
				break;
			}
			if (c == ',') {
				++pos;
				SkipWhitespace();
				if (Peek() == '}') {
					return std::nullopt;
				}
			} else {
				return std::nullopt;
			}
		}
		return utils::JSONValue(std::move(obj));
	}
	
	/**
	 * @brief Parse one arbitrary JSON value.
	 * @details Dispatches to specialized parsers based on next significant input
	 * character.
	 * @returns Parsed JSON value or `std::nullopt` if input is malformed.
	 */
	[[nodiscard]] std::optional<utils::JSONValue> ParseValue() {
		SkipWhitespace();
		char c = Peek();
		switch (c) {
			case 'n':
				if (input.substr(pos, 4) == "null") {
					pos += 4;
					return utils::JSONValue();
				}
				
				return std::nullopt;
			case 't':
				if (input.substr(pos, 4) == "true") {
					pos += 4;
					return utils::JSONValue(true);
				}
				
				return std::nullopt;
			case 'f':
				if (input.substr(pos, 5) == "false") {
					pos += 5;
					return utils::JSONValue(false);
				}
				
				return std::nullopt;
			case '"':
				if (auto parsed = ParseString()) {
					return utils::JSONValue(*parsed);
				}
				return std::nullopt;
			case '[':
				return ParseArray();
			case '{':
				return ParseObject();
			case '-':
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				return ParseNumber();
			default:
				return std::nullopt;
		}
	}
};

} // namespace

std::optional<ParseResult> Parse(std::string_view input) {
	ParseResult result{utils::JSONValue(), StringStorage{}};
	ParserState state{input, 0, result.string_storage};
	
	auto root = state.ParseValue();
	if (!root) {
		return std::nullopt;
	}
	
	state.SkipWhitespace();
	if (!state.AtEnd()) {
		return std::nullopt;
	}
	
	result.value = std::move(*root);
	return result;
}

} // namespace web_htop::json