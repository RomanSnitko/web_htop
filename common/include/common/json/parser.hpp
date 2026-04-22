/**
 * @file common/json/parser.hpp
 * 
 * @author Maksim Vashkevich
 * @date 2026-04-06
 * 
 * @brief JSON parser interface for the internal JSON model.
 * @details This header declares parser entry points and result structures used
 * to parse an input JSON document into `utils::JSONValue` while preserving
 * string storage lifetime for resulting `std::string_view` references.
 * Parser APIs are exception-free and report failures via `std::nullopt`.
 */

#ifndef WEB_HTOP_COMMON_JSON_PARSER_HPP_
#define WEB_HTOP_COMMON_JSON_PARSER_HPP_

#include <list>
#include <optional>
#include <string>
#include <string_view>

#include "utils.hpp"

namespace web_htop::json {

using StringStorage = std::list<std::string>; ///< Owning container for parsed JSON strings

/**
 * @brief Result of a successful JSON parse operation.
 * @details Contains parsed root value and owned string storage backing all
 * string views in the parsed tree.
 */
struct ParseResult {
    utils::JSONValue value;          ///< Parsed root JSON value
    StringStorage    string_storage; ///< Owning container for parsed JSON strings
};

/**
 * @brief Parse JSON text into internal representation.
 * @details Validates JSON syntax, decodes escaped sequences and constructs
 * an `utils::JSONValue` tree with stable string storage. The parser is
 * non-owning with respect to the input buffer: the `input` view must remain
 * valid for the full call duration. The parser does not throw exceptions and
 * reports any failure through `std::nullopt`.
 * @param input JSON text to parse.
 * @returns Parsed JSON value and string storage on success; `std::nullopt` if
 * input is malformed or not a complete JSON document, or if memory allocation fails.
 */
[[nodiscard]] std::optional<ParseResult> Parse(std::string_view input);

} // namespace web_htop::json

#endif // WEB_HTOP_COMMON_JSON_PARSER_HPP_