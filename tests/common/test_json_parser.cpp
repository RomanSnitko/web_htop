/**
 * @file tests/common/test_json_parser.cpp
 *
 * @author Maksim Vashkevich
 * @date 2026-04-13
 *
 * @brief Unit tests for JSON parser behavior and edge cases.
 * @details Covers successful parsing scenarios (primitives, arrays, objects,
 * numbers, escapes, Unicode and whitespace handling) and failure scenarios for
 * malformed JSON inputs.
 */

#include <gtest/gtest.h>

#include "common/json/parser.hpp"

#include <string>
#include <string_view>

namespace {

/**
 * @test JSONParserTest.VerifyPrimitiveJSONParsing
 * @brief Verifies parsing of primitive JSON field types in object context.
 * @details Checks unsigned integer, boolean, string and floating-point fields.
 */
TEST(JSONParserTest, VerifyPrimitiveJSONParsing) {
    auto parsed = web_htop::json::Parse(
        R"({"num":2281337666,"ok":true,"name":"compukter","some_ratio":50.05})"
    );
    ASSERT_TRUE(parsed.has_value());

    auto const & root = parsed->value;
    EXPECT_TRUE(root.IsObject());

    auto num = root["num"];
    ASSERT_TRUE(num.has_value());
    ASSERT_TRUE(num->get().AsUInt64().has_value());
    EXPECT_EQ(num->get().AsUInt64().value(), 2281337666ULL);

    auto ok = root["ok"];
    ASSERT_TRUE(ok.has_value());
    ASSERT_TRUE(ok->get().AsBool().has_value());
    EXPECT_TRUE(ok->get().AsBool().value());

    auto name = root["name"];
    ASSERT_TRUE(name.has_value());
    ASSERT_TRUE(name->get().AsString().has_value());
    EXPECT_EQ(name->get().AsString().value(), std::string_view("compukter"));

    auto ratio = root["some_ratio"];
    ASSERT_TRUE(ratio.has_value());
    ASSERT_TRUE(ratio->get().AsDouble().has_value());
    EXPECT_DOUBLE_EQ(ratio->get().AsDouble().value(), 50.05);
}

/**
 * @test JSONParserTest.VerifyEscapedAndUnicodeJSONParsing
 * @brief Verifies escape sequence and Unicode decoding in JSON strings.
 * @details Validates newline escape processing and UTF-16 surrogate pair decoding.
 */
TEST(JSONParserTest, VerifyEscapedAndUnicodeJSONParsing) {
    auto parsed = web_htop::json::Parse(
        R"({"random_text":"line\nA\u0042","random_emoji":"\uD83D\uDE00"})"
    );
    ASSERT_TRUE(parsed.has_value());

    auto const & root = parsed->value;

    auto text = root["random_text"];
    ASSERT_TRUE(text.has_value());
    ASSERT_TRUE(text->get().AsString().has_value());
    EXPECT_EQ(text->get().AsString().value(), std::string_view("line\nAB"));

    auto emoji = root["random_emoji"];
    ASSERT_TRUE(emoji.has_value());
    ASSERT_TRUE(emoji->get().AsString().has_value());
    EXPECT_EQ(emoji->get().AsString().value(), std::string_view("\xF0\x9F\x98\x80"));
}

/**
 * @test JSONParserTest.VerifyArrayParsing
 * @brief Verifies parsing and indexing behavior for JSON arrays.
 * @details Ensures array size, element values and out-of-range access handling.
 */
TEST(JSONParserTest, VerifyArrayParsing) {
    auto parsed = web_htop::json::Parse("[228,1337,666]");
    ASSERT_TRUE(parsed.has_value());

    auto const & root = parsed->value;
    EXPECT_TRUE(root.IsArray());
    EXPECT_EQ(root.size(), 3U);

    auto first = root[0];
    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(first->get().AsUInt64().has_value());
    EXPECT_EQ(first->get().AsUInt64().value(), 228ULL);

    auto second = root[1];
    ASSERT_TRUE(second.has_value());
    ASSERT_TRUE(second->get().AsUInt64().has_value());
    EXPECT_EQ(second->get().AsUInt64().value(), 1337ULL);

    auto third = root[2];
    ASSERT_TRUE(third.has_value());
    ASSERT_TRUE(third->get().AsUInt64().has_value());
    EXPECT_EQ(third->get().AsUInt64().value(), 666ULL);

    auto out_of_range = root[10];
    EXPECT_FALSE(out_of_range.has_value());
}

/**
 * @test JSONParserTest.VerifyArbitraryWhitespaceParsing
 * @brief Verifies parser tolerance to valid arbitrary whitespace.
 * @details Confirms correct parsing when spaces, tabs and newlines vary.
 */
TEST(JSONParserTest, VerifyArbitraryWhitespaceParsing) {
    auto parsed = web_htop::json::Parse(
        "{      \"num\"    :      1,\n"
        "   \"random_array\"  :   [   true,   false ,   null   ],\n"
        "\t\"ok\"    :    {   \"x\" :  \"ok\"   }\n"
        "      }"
    );
    ASSERT_TRUE(parsed.has_value());

    auto const & root = parsed->value;
    ASSERT_TRUE(root.IsObject());

    auto num = root["num"];
    ASSERT_TRUE(num.has_value());
    ASSERT_TRUE(num->get().AsUInt64().has_value());
    EXPECT_EQ(num->get().AsUInt64().value(), 1ULL);

    auto random_array = root["random_array"];
    ASSERT_TRUE(random_array.has_value());
    auto const * array = random_array->get().AsArray();
    ASSERT_NE(array, nullptr);
    ASSERT_EQ(array->size(), 3U);
    EXPECT_TRUE(array->at(0).AsBool().value());
    EXPECT_FALSE(array->at(1).AsBool().value());
    EXPECT_TRUE(array->at(2).IsNull());

    auto ok = root["ok"];
    ASSERT_TRUE(ok.has_value());
    auto const * obj = ok->get().AsObject();
    ASSERT_NE(obj, nullptr);
    ASSERT_EQ(obj->size(), 1U);
    EXPECT_EQ(obj->at(0).first, std::string_view("x"));
    EXPECT_EQ(obj->at(0).second.AsString().value(), std::string_view("ok"));
}

/**
 * @test JSONParserTest.VerifyRootLiteralsParsing
 * @brief Verifies valid JSON literals as root values.
 * @details Checks `null`, `true` and `false` root-level parsing.
 */
TEST(JSONParserTest, VerifyRootLiteralsParsing) {
    auto parsed_null = web_htop::json::Parse("null");
    ASSERT_TRUE(parsed_null.has_value());
    EXPECT_TRUE(parsed_null->value.IsNull());

    auto parsed_true = web_htop::json::Parse("true");
    ASSERT_TRUE(parsed_true.has_value());
    ASSERT_TRUE(parsed_true->value.AsBool().has_value());
    EXPECT_TRUE(parsed_true->value.AsBool().value());

    auto parsed_false = web_htop::json::Parse("false");
    ASSERT_TRUE(parsed_false.has_value());
    ASSERT_TRUE(parsed_false->value.AsBool().has_value());
    EXPECT_FALSE(parsed_false->value.AsBool().value());
}

/**
 * @test JSONParserTest.VerifyEmptyContainersParsing
 * @brief Verifies parsing of empty JSON object and array.
 * @details Ensures container type detection and zero element counts.
 */
TEST(JSONParserTest, VerifyEmptyContainersParsing) {
    auto parsed_object = web_htop::json::Parse("{}");
    ASSERT_TRUE(parsed_object.has_value());
    EXPECT_TRUE(parsed_object->value.IsObject());
    EXPECT_EQ(parsed_object->value.size(), 0U);

    auto parsed_array = web_htop::json::Parse("[]");
    ASSERT_TRUE(parsed_array.has_value());
    EXPECT_TRUE(parsed_array->value.IsArray());
    EXPECT_EQ(parsed_array->value.size(), 0U);
}

/**
 * @test JSONParserTest.VerifyFloatingPointAndSpecialNumbersParsing
 * @brief Verifies floating-point and exponent number parsing semantics.
 * @details Covers exponent notation, signed zero and underflow-to-zero behavior.
 */
TEST(JSONParserTest, VerifyFloatingPointAndSpecialNumbersParsing) {
    auto parsed = web_htop::json::Parse(R"({"a":1e3,"b":-2E-2,"c":0.0})");
    ASSERT_TRUE(parsed.has_value());

    auto const & root = parsed->value;
    auto a = root["a"];
    ASSERT_TRUE(a.has_value());
    ASSERT_TRUE(a->get().AsDouble().has_value());
    EXPECT_DOUBLE_EQ(a->get().AsDouble().value(), 1000.0);

    auto b = root["b"];
    ASSERT_TRUE(b.has_value());
    ASSERT_TRUE(b->get().AsDouble().has_value());
    EXPECT_DOUBLE_EQ(b->get().AsDouble().value(), -0.02);

    auto c = root["c"];
    ASSERT_TRUE(c.has_value());
    ASSERT_TRUE(c->get().AsDouble().has_value());
    EXPECT_DOUBLE_EQ(c->get().AsDouble().value(), 0.0);

    auto minus_zero = web_htop::json::Parse("-0");
    ASSERT_TRUE(minus_zero.has_value());
    ASSERT_TRUE(minus_zero->value.AsInt64().has_value());
    EXPECT_EQ(minus_zero->value.AsInt64().value(), 0LL);

    auto underflow_exp = web_htop::json::Parse("1e-1000");
    ASSERT_TRUE(underflow_exp.has_value());
    ASSERT_TRUE(underflow_exp->value.AsDouble().has_value());
    EXPECT_DOUBLE_EQ(underflow_exp->value.AsDouble().value(), 0.0);
}

/**
 * @test JSONParserTest.VerifyNumericBoundariesAndIncompatibleAccessors
 * @brief Verifies numeric boundary parsing and typed accessor contracts.
 * @details Checks int/uint limits and expected `std::nullopt` for invalid conversions.
 */
TEST(JSONParserTest, VerifyNumericBoundariesAndIncompatibleAccessors) {
    auto parsed = web_htop::json::Parse(
        R"({"i64_min":-9223372036854775808,"i64_max":9223372036854775807,"u64_max":18446744073709551615,"neg":-1,"flt":1.5,"text":"x"})"
    );
    ASSERT_TRUE(parsed.has_value());

    auto const & root = parsed->value;

    auto i64_min = root["i64_min"];
    ASSERT_TRUE(i64_min.has_value());
    ASSERT_TRUE(i64_min->get().AsInt64().has_value());
    EXPECT_EQ(i64_min->get().AsInt64().value(), -9223372036854775807LL - 1LL);

    auto i64_max = root["i64_max"];
    ASSERT_TRUE(i64_max.has_value());
    ASSERT_TRUE(i64_max->get().AsInt64().has_value());
    EXPECT_EQ(i64_max->get().AsInt64().value(), 9223372036854775807LL);

    auto u64_max = root["u64_max"];
    ASSERT_TRUE(u64_max.has_value());
    ASSERT_TRUE(u64_max->get().AsUInt64().has_value());
    EXPECT_EQ(u64_max->get().AsUInt64().value(), 18446744073709551615ULL);

    auto neg = root["neg"];
    ASSERT_TRUE(neg.has_value());
    EXPECT_FALSE(neg->get().AsUInt64().has_value());

    EXPECT_FALSE(u64_max->get().AsInt64().has_value());

    auto flt = root["flt"];
    ASSERT_TRUE(flt.has_value());
    EXPECT_FALSE(flt->get().AsUInt64().has_value());
    EXPECT_FALSE(flt->get().AsInt64().has_value());

    auto text = root["text"];
    ASSERT_TRUE(text.has_value());
    EXPECT_FALSE(text->get().AsDouble().has_value());
}

/**
 * @test JSONParserTest.RejectMalformedNumbers
 * @brief Verifies rejection of syntactically invalid or overflowing numbers.
 * @details Ensures parser reports failure for malformed numeric tokens.
 */
TEST(JSONParserTest, RejectMalformedNumbers) {
    EXPECT_FALSE(web_htop::json::Parse(R"({"n":01})").has_value());
    EXPECT_FALSE(web_htop::json::Parse(R"({"n":1.})").has_value());
    EXPECT_FALSE(web_htop::json::Parse(R"({"n":.5})").has_value());
    EXPECT_FALSE(web_htop::json::Parse(R"({"n":1e})").has_value());
    EXPECT_FALSE(web_htop::json::Parse(R"({"n":1e+})").has_value());
    EXPECT_FALSE(web_htop::json::Parse(R"({"n":18446744073709551617})").has_value());
}

/**
 * @test JSONParserTest.RejectMalformedStringsAndUnicode
 * @brief Verifies rejection of invalid string escapes and Unicode forms.
 * @details Covers illegal escapes, invalid surrogates and unescaped control bytes.
 */
TEST(JSONParserTest, RejectMalformedStringsAndUnicode) {
    EXPECT_FALSE(web_htop::json::Parse("{\"s\":\"\\q\"}").has_value());
    EXPECT_FALSE(web_htop::json::Parse("{\"s\":\"\\uD800\"}").has_value());
    EXPECT_FALSE(web_htop::json::Parse("{\"s\":\"\\uDC00\"}").has_value());
    EXPECT_FALSE(web_htop::json::Parse(R"({"s":"\uD83D\uD83D"})").has_value());
    EXPECT_FALSE(web_htop::json::Parse("{\"s\":\"unterminated}").has_value());
    std::string const contains_soh = "{\"a\":\"\x01\"}";
    std::string const contains_us  = "{\"a\":\"\x1F\"}";

    EXPECT_FALSE(web_htop::json::Parse(contains_soh).has_value());
    EXPECT_FALSE(web_htop::json::Parse(contains_us).has_value());
}

/**
 * @test JSONParserTest.RejectMalformedStructure
 * @brief Verifies rejection of malformed JSON document structure.
 * @details Checks trailing commas, missing separators and trailing garbage input.
 */
TEST(JSONParserTest, RejectMalformedStructure) {
    EXPECT_FALSE(web_htop::json::Parse(R"({"a":1,})").has_value());
    EXPECT_FALSE(web_htop::json::Parse(R"([1,2,])").has_value());
    EXPECT_FALSE(web_htop::json::Parse(R"({"a" 1})").has_value());
    EXPECT_FALSE(web_htop::json::Parse(R"({"a":1} trailing)").has_value());
}

} // namespace