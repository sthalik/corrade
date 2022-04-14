#ifndef Corrade_Utility_Json_h
#define Corrade_Utility_Json_h
/*
    This file is part of Corrade.

    Copyright © 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016,
                2017, 2018, 2019, 2020, 2021, 2022
              Vladimír Vondruš <mosra@centrum.cz>

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

/** @file
 * @brief Class @ref Corrade::Utility::Json, @ref Corrade::Utility::JsonToken
 * @m_since_latest
 */

#include <cstdint>

#include "Corrade/Containers/Pointer.h"
#include "Corrade/Containers/EnumSet.h"
#include "Corrade/Utility/visibility.h"

namespace Corrade { namespace Utility {

/**
@brief JSON parser
@m_since_latest

Tokenizes a JSON file together with optional parsing of selected token
subtrees. Supports files over 4 GB and parsing of numeric values into 32-bit
floating-point, 32-bit and 52-/53-bit unsigned and signed integer types in
addition to the general 64-bit floating-point representation.

To optimize for parsing performance and minimal memory usage, the parsed tokens
are contained in a single contiguous allocation and form an immutable view on
the input JSON string. As the intended usage is sequential processing of chosen
parts of the file, there's no time spent building any acceleration structures
for fast lookup of keys and array indices --- if that's desired, users are
encouraged to build them on top of the parsed output.

The @ref JsonWriter class provides a write-only counterpart for saving a JSON
file.

@section Utility-Json-usage Usage

The following snippet opens a very minimal
[glTF](https://www.khronos.org/gltf/) file, parses it including unescaping
strings and converting numbers to floats, and accesses the known properties:

@m_class{m-row m-container-inflate}

@parblock

@m_div{m-col-l-4}
@code{.json}
{
  "asset": {
    "version": "2.0"
  },
  "nodes": [
    …,
    {
      "name": "Fox",
      "mesh": 5
    }
  ]
}
@endcode
@m_enddiv

@m_div{m-col-l-8}
@snippet Utility.cpp Json-usage
@m_enddiv

@endparblock

The above --- apart from handling a parsing failure --- assumes that the node
`i` we're looking for exists in the file and contains the properties we want.
But that might not always be the case and so it could assert if we'd ask for
something that's not there. This is how it would look with
@ref JsonToken::find() used instead of @ref JsonToken::operator[]():

@snippet Utility.cpp Json-usage-find

The next level of error handling would be for an invalid glTF file, for example
where the root element is not an object, or the name isn't a string. Those
would still cause the above code to assert, if a graceful handling is desired
instead, the code either has to check the @ref JsonToken::type() or call into
@ref JsonToken::parseString() etc. instead of @relativeref{JsonToken,asString()}
etc.:

@snippet Utility.cpp Json-usage-checks

Furthermore, while numeric values in a JSON are defined to be of a
floating-point type, often the values are meant to represent integer sizes or
offsets --- such as here in case of the mesh index. Calling
@ref JsonToken::parseUnsignedInt() instead of
@relativeref{JsonToken,parseFloat()} / @relativeref{JsonToken,asFloat()} will
not only check that it's a numeric value, but also that it has no fractional
part and is not negative:

@snippet Utility.cpp Json-usage-checks2

@subsection Utility-Json-usage-iteration Iterating objects and arrays

While the above works sufficiently well for simple use cases, the parser and
internal representation is optimized for linear consumption rather than lookup
by keys or values --- those are @f$ \mathcal{O}(n) @f$ operations here. The
preferred way to consume a parsed @ref Json instance is thus by iterating over
object and array contents using @ref JsonToken::asObject() and
@relativeref{JsonToken,asArray()} and building your own representation from
these:

@snippet Utility.cpp Json-usage-iteration

Both @ref JsonObjectItem and @ref JsonArrayItem is implicitly convertible to a
@ref JsonToken reference if the array index or object key wouldn't be needed
in the loop:

@snippet Utility.cpp Json-usage-iteration-values

@subsection Utility-Json-usage-selective-parsing Selective parsing

In the snippets above, the @ref Options passed to @ref fromFile() or
@ref fromString() caused the file to get fully parsed upfront including number
conversion and string unescaping. While that's a convenient behavior that
makes sense when consuming the whole file, doing all that might be unnecessary
when you only need to access a small portion of a large file. In the following
snippet, only string keys get parsed upfront so objects can be searched, but
then we parse only contents of the particular node using
@ref parseLiterals() and @ref parseStrings():

@snippet Utility.cpp Json-usage-selective-parsing

This way we also have a greater control over parsed numeric types. Commonly a
JSON file contains a mixture of floating-point and integer numbers and thus a
top-level @ref Option for parsing everything as integers makes little sense.
But on a token level it's clearer which values are meant to be integers and
which floats and so we can choose among @ref parseFloats(),
@ref parseUnsignedInts(), @ref parseInts() etc. Similarly as with on-the-fly
parsing of a single value with @ref JsonToken::parseUnsignedInt() shown above,
these will produce errors when floating-point values appear where integers were
expected:

@snippet Utility.cpp Json-usage-selective-parsing2

While a JSON number is defined to be of a @cpp double @ce type, often the full
precision is not needed. If it indeed is, you can switch to
@ref Option::ParseDoubles, @ref parseDoubles() or @ref JsonToken::parseDouble()
instead of @ref Option::ParseFloats or @ref parseFloats() where needed.

@subsection Utility-Json-usage-direct-array-access Direct access to numeric arrays

Besides high-level array iteration, there's also a set of function for
accessing homogeneous numeric arrays. Coming back to the glTF format, for
example a node translation vector and child indices:

@code{.json}
{
  "translation": [1.5, -0.5, 2.3],
  "children": [2, 3, 4, 17, 399]
}
@endcode

We'll parse and access the first property with @ref JsonToken::asFloatArray()
and the other with @relativeref{JsonToken,asUnsignedIntArray()}, again using
that to catch unexpected non-index values in the children array. If the parsed
type wouldn't match, if there would be unparsed numeric values left or if the
array would contain other things than just numbers, the functions return
@ref Containers::NullOpt.

@snippet Utility.cpp Json-usage-direct-array-access

@section Utility-Json-tokenization Tokenization and parsing process

The class expects exactly one top-level JSON value, be it an object, array,
literal, number or a string.

The tokenization process is largely inspired by [jsmn](https://github.com/zserge/jsmn)
--- the file gets processed to a flat list of @ref JsonToken instances, where
each literal, number, string (or a string object key), object and array is one
token, ordered in a depth-first manner. Whitespace is skipped and not present
in the parsed token list. @ref JsonToken::data() is a view on the input string
that defines the token, with the range containing also all nested tokens for
objects and arrays. @ref JsonToken::type() is then implicitly inferred from the
first byte of the token, but no further parsing or validation of actual token
values is done during the initial tokenization.

This allows the application to reduce the initial processing time and memory
footprint. Token parsing is a subsequent step, parsing the string range of a
token as a literal, converting it to a number or interpreting string escape
sequences. This step can then fail on its own, for example when an invalid
literal value is encountered, when a Unicode escape is out of bounds or when a
parsed integer doesn't fit into the output type size.

Token hierarchy is defined as the following --- object tokens have string keys
as children, string keys have object values as children, arrays have array
values as children and values themselves have no children. As implied by the
depth-first ordering, the first child token (if any) is ordered right after
its parent token, and together with @ref JsonToken::childCount(), which is the
count of all nested tokens, it's either possible to dive into the child token
tree using @ref JsonToken::firstChild() or @ref JsonToken::children() or skip
after the child token tree using @ref JsonToken::next().

@section Utility-Json-representation Internal representation

If the string passed to @ref fromString() is
@ref Containers::StringViewFlag::Global, it's just referenced without an
internal copy, and all token data will point to it as well. Otherwise, or if
@ref fromFile() is used, a local copy is made, and tokens point to the copy
instead.

A @ref JsonToken is 16 bytes on 32-bit systems and 24 bytes on 64-bit systems,
containing view pointer, size and child count. When a literal or numeric value
is parsed, it's stored inside. Simply put, the representation exploits the
fact that a token either has children or is a value, but never both. For
strings the general assumption is that most of them (and especially object
keys) don't contain any escape characters and thus can be returned as views on
the input string. Strings containing escape characters are parsed on-demand and
allocated separately.
*/
class CORRADE_UTILITY_EXPORT Json {
    public:
        /**
         * @brief Parsing option
         *
         * @see @ref Options, @ref fromString(), @ref fromFile()
         */
        enum class Option {
            /**
             * Parse the @cb{.json} null @ce, @cb{.json} true @ce and
             * @cb{.json} false @ce values. Causes all @ref JsonToken instances
             * of @ref JsonToken::Type::Null and @ref JsonToken::Type::Bool to
             * have @ref JsonToken::isParsed() set and be accessible through
             * @ref JsonToken::asNull() and @ref JsonToken::asBool().
             *
             * Invalid values will cause @ref fromString() / @ref fromFile()
             * to print an error and return @ref Containers::NullOpt. This
             * operation can be also performed selectively later using
             * @ref parseLiterals(), or on-the-fly for particular tokens using
             * @ref JsonToken::parseBool().
             */
            ParseLiterals = 1 << 0,

            /**
             * Parse all numbers as 64-bit floating-point values. Causes all
             * @ref JsonToken instances of @ref JsonToken::Type::Number to
             * become @ref JsonToken::ParsedType::Double and be accessible
             * through @ref JsonToken::asDouble(). If both
             * @ref Option::ParseDoubles and @ref Option::ParseFloats is
             * specified, @ref Option::ParseDoubles gets a precedence.
             *
             * Invalid values will cause @ref fromString() / @ref fromFile()
             * to exit with a parse error. This operation can be also performed
             * selectively later using @ref parseDoubles(), or on-the-fly for
             * particular tokens using @ref JsonToken::parseDouble().
             *
             * While this option is guaranteed to preserve the full precision
             * of JSON numeric literals, often you may need only 32-bit
             * precision --- use @ref Option::ParseFloats in that case instead.
             * It's also possible to selectively parse certain values as
             * integers using @ref parseUnsignedInts(), @ref parseInts(),
             * @ref parseUnsignedLongs(), @ref parseLongs() or
             * @ref parseSizes(), then the parsing will also check that the
             * value is an (unsigned) integer and can be represented in given
             * type size. There's no corresponding @ref Option for those, as
             * JSON files rarely contain just integer numbers alone. If you
             * indeed have an integer-only file, call those directly on
             * @ref root().
             */
            ParseDoubles = 1 << 1,

            /**
             * Parse all numbers as 32-bit floating point values. Causes all
             * @ref JsonToken instances of @ref JsonToken::Type::Number to
             * become @ref JsonToken::ParsedType::Float and be accessible
             * through @ref JsonToken::asFloat(). If both
             * @ref Option::ParseDoubles and @ref Option::ParseFloats is
             * specified, @ref Option::ParseDoubles gets a precedence.
             *
             * Invalid values will cause @ref fromString() / @ref fromFile()
             * to exit with a parse error. This operation can be also performed
             * selectively later using @ref parseFloats(), or on-the-fly for
             * particular tokens using @ref JsonToken::parseFloat().
             *
             * While 32-bit float precision is often enough, sometimes you
             * might want to preserve the full precision of JSON numeric
             * literals --- use @ref Option::ParseDoubles in that case instead.
             * It's also possible to selectively parse certain values as
             * integers using @ref parseUnsignedInts(), @ref parseInts(),
             * @ref parseUnsignedLongs(), @ref parseLongs() or
             * @ref parseSizes(), then the parsing will also check that the
             * value is an (unsigned) integer and can be represented in given
             * type size. There's no corresponding @ref Option for those, as
             * JSON files rarely contain just integer numbers alone. If you
             * indeed have an integer-only file, call those directly on
             * @ref root().
             */
            ParseFloats = 1 << 2,

            /**
             * Parse object key strings by processing all escape sequences and
             * caching the parsed result (or marking the original string as
             * parsed in-place, if it has no escape sequences). Causes
             * @ref JsonToken instances of @ref JsonToken::Type::String that
             * are children of a @ref JsonToken::Type::Object to have
             * @ref JsonToken::isParsed() set and be accessible through
             * @ref JsonToken::asString(). String values (as opposed to
             * keys) are left untouched, thus this is useful if you need to
             * perform a key-based search, but don't need to have also all
             * other strings unescaped.
             *
             * Invalid values will cause @ref fromString() / @ref fromFile()
             * to exit with a parse error. This operation can be also performed
             * selectively later using @ref parseStringKeys(), or on-the-fly
             * for particular tokens using @ref JsonToken::parseString().
             */
            ParseStringKeys = 1 << 3,

            /**
             * Parse string values by processing all escape sequences and
             * caching the parsed result (or marking the original string as
             * parsed in-place, if it has no escape sequences). Causes all
             * @ref JsonToken instances of @ref JsonToken::Type::String to have
             * @ref JsonToken::isParsed() set and be accessible through
             * @ref JsonToken::asString(). Implies
             * @ref Option::ParseStringKeys.
             *
             * Invalid values will cause @ref fromString() / @ref fromFile()
             * to exit with a parse error. This operation can be also performed
             * selectively later using @ref parseStrings(), or on-the-fly for
             * particular tokens using @ref JsonToken::parseString().
             */
            ParseStrings = ParseStringKeys|(1 << 4)
        };

        /**
         * @brief Parsing options
         *
         * @see @ref fromString(), @ref fromFile()
         */
        typedef Containers::EnumSet<Option> Options;
        CORRADE_ENUMSET_FRIEND_OPERATORS(Options)

        /**
         * @brief Parse a JSON string
         *
         * By default performs only tokenization, not parsing any literals. Use
         * @p options to enable parsing of particular token types as well. If
         * a tokenization or parsing error happens, prints an error and returns
         * @ref Containers::NullOpt.
         *
         * If the @p string is @ref Containers::StringViewFlag::Global, the
         * parsed tokens will reference it, returning also global string
         * literals. Otherwise a copy is made internally.
         */
        #ifdef DOXYGEN_GENERATING_OUTPUT
        static Containers::Optional<Json> fromString(Containers::StringView string, Options options = {});
        #else
        /* For binary size savings -- the base implementation does only minimal
           tokenization and thus the parsing code can be DCE'd if unused */
        static Containers::Optional<Json> fromString(Containers::StringView string);
        static Containers::Optional<Json> fromString(Containers::StringView string, Options options);
        #endif

        /**
         * @brief Parse a JSON file
         *
         * By default performs only tokenization, not parsing any literals. Use
         * @p options to enable parsing of particular token types as well. If
         * the file can't be read, or a tokenization or parsing error happens,
         * prints an error and returns @ref Containers::NullOpt.
         */
        #ifdef DOXYGEN_GENERATING_OUTPUT
        static Containers::Optional<Json> fromFile(Containers::StringView filename, Options options = {});
        #else
        /* These contain the same code and delegate to either the base
           implementation or the complex one, allowing the extra code to be
           DCE'd */
        static Containers::Optional<Json> fromFile(Containers::StringView filename);
        static Containers::Optional<Json> fromFile(Containers::StringView filename, Options options);
        #endif

        /** @brief Copying is not allowed */
        Json(const Json&) = delete;

        /** @brief Move constructor */
        Json(Json&&) noexcept;

        ~Json();

        /** @brief Copying is not allowed */
        Json& operator=(const Json&) = delete;

        /** @brief Move assignment */
        Json& operator=(Json&&) noexcept;

        /**
         * @brief Parsed JSON tokens
         *
         * The first token is the root token (also accessible via @ref root())
         * and is always present, the rest is ordered in a depth-first manner
         * as described in @ref Utility-Json-tokenization.
         */
        Containers::ArrayView<const JsonToken> tokens() const;

        /**
         * @brief Root JSON token
         *
         * Always present. Tts @ref JsonToken::children() (if any) contain the
         * whole document ordered in a depth-first manner as described in
         * @ref Utility-Json-tokenization.
         */
        const JsonToken& root() const;

        /**
         * @brief Parse `null`, `true` and `false` literals in given token tree
         *
         * Causes all @ref JsonToken instances of @ref JsonToken::Type::Null
         * and @ref JsonToken::Type::Bool in @p token and its children to have
         * @ref JsonToken::isParsed() set and be accessible through
         * @ref JsonToken::asNull() and @ref JsonToken::asBool(). Non-literal
         * tokens and tokens that are already parsed are skipped. If an invalid
         * value is encountered, prints an error and returns @cpp false @ce.
         *
         * Passing @ref root() as @p token has the same effect as
         * @ref Option::ParseLiterals specified during the initial
         * @ref fromString() or @ref fromFile() call. A single token can be
         * also parsed on-the-fly using @ref JsonToken::parseNull() or
         * @ref JsonToken::parseBool().
         */
        bool parseLiterals(const JsonToken& token);

        /**
         * @brief Parse numbers in given token tree as 64-bit floating-point values
         *
         * Causes all @ref JsonToken instances of @ref JsonToken::Type::Number
         * in @p token and its children to become
         * @ref JsonToken::ParsedType::Double and be accessible through
         * @ref JsonToken::asDouble(). Non-numeric tokens and numeric tokens
         * that are already parsed as doubles are skipped, numeric tokens
         * parsed as other types are reparsed. If an invalid value is
         * encountered, prints an error and returns @cpp false @ce.
         *
         * Passing @ref root() as @p token has the same effect as
         * @ref Option::ParseDoubles specified during the initial
         * @ref fromString() or @ref fromFile() call. A single token can be
         * also parsed on-the-fly using @ref JsonToken::parseDouble().
         * @see @ref parseFloats(), @ref parseUnsignedInts(), @ref parseInts()
         */
        bool parseDoubles(const JsonToken& token);

        /**
         * @brief Parse numbers in given token tree as 32-bit floating-point values
         *
         * Causes all @ref JsonToken instances of @ref JsonToken::Type::Number
         * in @p token and its children to become
         * @ref JsonToken::ParsedType::Float and be accessible through
         * @ref JsonToken::asFloat(). Non-numeric tokens and numeric tokens
         * that are already parsed as floats are skipped, numeric tokens parsed
         * as other types are reparsed. If an invalid value is encountered,
         * prints an error and returns @cpp false @ce.
         *
         * Passing @ref root() as @p token has the same effect as
         * @ref Option::ParseFloats specified during the initial
         * @ref fromString() or @ref fromFile() call. A single token can be
         * also parsed on-the-fly using @ref JsonToken::parseFloat().
         * @see @ref parseDoubles(), @ref parseUnsignedInts(), @ref parseInts()
         */
        bool parseFloats(const JsonToken& token);

        /**
         * @brief Parse numbers in given token tree as unsigned 32-bit integer values
         *
         * Causes all @ref JsonToken instances of @ref JsonToken::Type::Number
         * in @p token and its children to become
         * @ref JsonToken::ParsedType::UnsignedInt and be accessible through
         * @ref JsonToken::asUnsignedInt(). Non-numeric tokens and numeric
         * tokens that are already parsed as unsigned int are skipped, numeric
         * tokens parsed as other types are reparsed. If an invalid value,
         * a literal with a fractional or exponent part or a negative value is
         * encountered or a value doesn't fit into a 32-bit representation,
         * prints an error and returns @cpp false @ce.
         *
         * A single token can be also parsed on-the-fly using
         * @ref JsonToken::parseUnsignedInt().
         * @see @ref parseDoubles(), @ref parseInts(),
         *      @ref parseUnsignedLongs(), @ref parseSizes()
         */
        bool parseUnsignedInts(const JsonToken& token);

        /**
         * @brief Parse numbers in given token tree as signed 32-bit integer values
         *
         * Causes all @ref JsonToken instances of @ref JsonToken::Type::Number
         * in @p token and its children to become
         * @ref JsonToken::ParsedType::Int and be accessible through
         * @ref JsonToken::asInt(). Non-numeric tokens and numeric tokens that
         * are already parsed as int are skipped, numeric tokens parsed as
         * other types are reparsed. If an invalid value, a literal with a
         * fractional or exponent part is encountered or a value doesn't fit
         * into a 32-bit representation, prints an error and returns
         * @cpp false @ce.
         *
         * A single token can be also parsed on-the-fly using
         * @ref JsonToken::parseInt().
         * @see @ref parseDoubles(), @ref parseUnsignedInts(),
         *      @ref parseLongs(), @ref parseSizes()
         */
        bool parseInts(const JsonToken& token);

        /**
         * @brief Parse numbers in given token tree as unsigned 52-bit integer values
         *
         * Causes all @ref JsonToken instances of @ref JsonToken::Type::Number
         * in @p token and its children to become
         * @ref JsonToken::ParsedType::UnsignedLong and be accessible through
         * @ref JsonToken::asUnsignedLong(). Non-numeric tokens and numeric
         * tokens that are already parsed as unsigned long are skipped, numeric
         * tokens parsed as other types are reparsed. If an invalid value, a
         * literal with a fractional or exponent part or a negative value is
         * encountered or a value doesn't fit into 52 bits (which is the
         * representable unsigned integer range in a JSON), prints an error and
         * returns @cpp false @ce.
         *
         * A single token can be also parsed on-the-fly using
         * @ref JsonToken::parseUnsignedLong().
         * @see @ref parseDoubles(), @ref parseLongs(),
         *      @ref parseUnsignedInts(), @ref parseSizes()
         */
        bool parseUnsignedLongs(const JsonToken& token);

        #ifndef CORRADE_TARGET_32BIT
        /**
         * @brief Parse numbers in given token tree as signed 53-bit integer values
         *
         * Causes all @ref JsonToken instances of @ref JsonToken::Type::Number
         * in @p token and its children to become
         * @ref JsonToken::ParsedType::Long and be accessible through
         * @ref JsonToken::asLong(). Non-numeric tokens and numeric tokens that
         * are already parsed as long are skipped, numeric tokens parsed as
         * other types are reparsed. If an invalid value, a literal with a
         * fractional or exponent part is encountered or a value doesn't fit
         * into 53 bits (which is the representable signed integer range in a
         * JSON), prints an error and returns @cpp false @ce.
         *
         * Available only on 64-bit targets due to limits of the internal
         * representation. On @ref CORRADE_TARGET_32BIT "32-bit targets" you
         * can use either @ref parseInts(), @ref parseDoubles() or parse the
         * integer value always on-the-fly using
         * @ref JsonToken::parseLong().
         * @see @ref parseDoubles(), @ref parseUnsignedLongs(),
         *      @ref parseInts(), @ref parseSizes()
         */
        bool parseLongs(const JsonToken& token);
        #endif

        /**
         * @brief Parse numbers in given token tree as size values
         *
         * Convenience function that calls into @ref parseUnsignedInts() on
         * @ref CORRADE_TARGET_32BIT "32-bit targets" and into
         * @ref parseUnsignedLongs() on 64-bit. Besides being available under
         * the concrete types as documented in these functions, @ref JsonToken
         * instances of @ref JsonToken::Type::Number in @p token and its
         * children will alias to @ref JsonToken::ParsedType::Size and be also
         * accessible through @ref JsonToken::asSize().
         */
        bool parseSizes(const JsonToken& token);

        /**
         * @brief Parse string keys in given token tree
         *
         * Causes all @ref JsonToken instances of @ref JsonToken::Type::String
         * that are children of a @ref JsonToken::Type::Object in @p token and
         * its children to have @ref JsonToken::isParsed() set and be
         * accessible through @ref JsonToken::asString(). The operation is a
         * subset of @ref parseStringKeys(). Non-string tokens, string tokens
         * that are not object keys and string tokens that are already parsed
         * are skipped. If an invalid value is encountered, prints an error and
         * returns @cpp false @ce.
         *
         * Passing @ref root() as @p token has the same effect as
         * @ref Option::ParseStringKeys specified during the initial
         * @ref fromString() or @ref fromFile() call. A single token can be
         * also parsed on-the-fly using @ref JsonToken::parseString().
         */
        bool parseStringKeys(const JsonToken& token);

        /**
         * @brief Parse strings in given token tree
         *
         * Causes all @ref JsonToken instances of @ref JsonToken::Type::String
         * in @p token and its children to have @ref JsonToken::isParsed() set
         * and be accessible through @ref JsonToken::asString(). The operation
         * is a superset of @ref parseStringKeys(). Non-string tokens and
         * string tokens that are already parsed are skipped. If an invalid
         * value is encountered, prints an error and returns @cpp false @ce.
         *
         * Passing @ref root() as @p token has the same effect as
         * @ref Option::ParseStrings specified during the initial
         * @ref fromString() or @ref fromFile() call. A single token can be
         * also parsed on-the-fly using @ref JsonToken::parseString().
         */
        bool parseStrings(const JsonToken& token);

    private:
        struct State;

        explicit CORRADE_UTILITY_LOCAL Json();

        /* These are here because they need friended JsonToken */
        CORRADE_UTILITY_LOCAL static Containers::Optional<Json> tokenize(Containers::StringView filename, Containers::StringView string);
        CORRADE_UTILITY_LOCAL static Containers::Optional<Json> tokenize(Containers::StringView filename, Containers::StringView string, Options options);

        Containers::Pointer<State> _state;
};

/**
@brief A single JSON token
@m_since_latest

Represents an object, array, null, boolean, numeric or a string value in a JSON
file. See the @ref Json class documentation for more information.
*/
class CORRADE_UTILITY_EXPORT JsonToken {
    public:
        /**
         * @brief Token type
         *
         * @see @ref type()
         */
        enum class Type: std::uint64_t {
            /* Needs to match the private flags */

            /**
             * An object, @cb{.json} {} @ce. Its immediate children are
             * @ref Type::String keys, values are children of the keys. The
             * keys can be in an arbitrary order and can contain duplicates.
             * @ref isParsed() is set always.
             * @see @ref children(), @ref firstChild(), @ref next()
             */
            #ifndef CORRADE_TARGET_32BIT
            Object
                #ifndef DOXYGEN_GENERATING_OUTPUT
                = 1ull << 61
                #endif
                ,
            #else
            Object = 1ull << 49,
            #endif

            /**
             * An array, @cb{.json} [] @ce. Its immediate children are values.
             * @ref isParsed() is set always.
             * @see @ref children(), @ref firstChild(), @ref next()
             */
            #ifndef CORRADE_TARGET_32BIT
            Array
                #ifndef DOXYGEN_GENERATING_OUTPUT
                = 2ull << 61
                #endif
                ,
            #else
            Array = 2ull << 49,
            #endif

            /**
             * A @cb{.json} null @ce value. Unless @ref isParsed() is set, the
             * value is not guaranteed to be valid.
             * @see @ref parseNull(), @ref asNull(),
             *      @ref Json::Option::ParseLiterals,
             *      @ref Json::parseLiterals()
             */
            #ifndef CORRADE_TARGET_32BIT
            Null
                #ifndef DOXYGEN_GENERATING_OUTPUT
                = 3ull << 61
                #endif
                ,
            #else
            Null = 3ull << 49,
            #endif

            /**
             * A @cb{.json} true @ce or @cb{.json} false @ce value. Unless
             * @ref isParsed() is set, the value is not guaranteed to be valid.
             * @see @ref parseBool(), @ref asBool(),
             *      @ref Json::Option::ParseLiterals,
             *      @ref Json::parseLiterals()
             */
            #ifndef CORRADE_TARGET_32BIT
            Bool
                #ifndef DOXYGEN_GENERATING_OUTPUT
                = 4ull << 61
                #endif
                ,
            #else
            Bool = 4ull << 49,
            #endif

            /**
             * A number. Unless @ref isParsed() is set, the value is not
             * guaranteed to be valid. JSON numbers are always 64-bit floating
             * point values but you can choose whether to parse them as doubles
             * or floats using @ref parseDouble() or @ref parseFloat(). If an
             * integer value is expected you can use @ref parseInt(),
             * @ref parseUnsignedInt(), @ref parseLong(),
             * @ref parseUnsignedLong() or @ref parseSize() instead to
             * implicitly check that it has a zero fractional part or
             * additionally that it's also non-negative.
             * @see @ref asDouble(), @ref asFloat(), @ref asUnsignedInt(),
             *      @ref asInt(), @ref asUnsignedLong(), @ref asLong(),
             *      @ref asSize(), @ref Json::Option::ParseDoubles,
             *      @ref Json::Option::ParseFloats,
             *      @ref Json::parseDoubles(), @ref Json::parseFloats(),
             *      @ref Json::parseUnsignedInts(), @ref Json::parseInts(),
             *      @ref Json::parseUnsignedLongs(), @ref Json::parseLongs(),
             *      @ref Json::parseSizes()
             */
            #ifndef CORRADE_TARGET_32BIT
            Number
                #ifndef DOXYGEN_GENERATING_OUTPUT
                = 5ull << 61
                #endif
                ,
            #else
            Number = 5ull << 49,
            #endif

            /**
             * A string. Unless @ref isParsed() is set, the value is not
             * guaranteed to be valid.
             * @see @ref parseString(), @ref asString(),
             *      @ref Json::Option::ParseStringKeys,
             *      @ref Json::Option::ParseStrings,
             *      @ref Json::parseStringKeys(), @ref Json::parseStrings()
             */
            #ifndef CORRADE_TARGET_32BIT
            String
                #ifndef DOXYGEN_GENERATING_OUTPUT
                = 6ull << 61
                #endif
                ,
            #else
            String = 6ull << 49,
            #endif
        };

        /**
         * @brief Parsed type
         *
         * @see @ref parsedType()
         */
        enum class ParsedType: std::uint64_t {
            /** Not parsed yet. */
            None = 0,

            /**
             * 64-bit floating-point value.
             *
             * Set if @ref Json::Option::ParseDoubles is passed to
             * @ref Json::fromString() or @ref Json::fromFile() or if
             * @ref Json::parseDoubles() is called later.
             */
            #ifndef CORRADE_TARGET_32BIT
            Double
                #ifndef DOXYGEN_GENERATING_OUTPUT
                = 1ull << 58
                #endif
                ,
            #else
            Double = 1ull << 29,
            #endif

            /**
             * 32-bit floating-point value.
             *
             * Set if @ref Json::Option::ParseFloats is passed to
             * @ref Json::fromString() or @ref Json::fromFile() or if
             * @ref Json::parseFloats() is called later. Double-precision
             * values that can't be represented as a float are truncated.
             */
            #ifndef CORRADE_TARGET_32BIT
            Float
                #ifndef DOXYGEN_GENERATING_OUTPUT
                = 2ull << 58
                #endif
                ,
            #else
            Float = 2ull << 29,
            #endif

            /**
             * 32-bit unsigned integer value.
             *
             * Set if @ref Json::parseUnsignedInts() is called on a particular
             * subtree. Except for invalid values, parsing fails also if any
             * the values have a non-zero fractional part, if they have an
             * exponent, if they're negative or if they can't fit into 32 bits.
             * @see @ref ParsedType::Size, @ref Json::parseSizes()
             */
            #ifndef CORRADE_TARGET_32BIT
            UnsignedInt
                #ifndef DOXYGEN_GENERATING_OUTPUT
                = 3ull << 58
                #endif
                ,
            #else
            UnsignedInt = 3ull << 29,
            #endif

            /**
             * 32-bit signed integer value.
             *
             * Set if @ref Json::parseInts() is called on a particular subtree.
             * Except for invalid values, parsing fails also if any the values
             * have a non-zero fractional part, if they have an exponent or if
             * they can't fit into 32 bits.
             */
            #ifndef CORRADE_TARGET_32BIT
            Int
                #ifndef DOXYGEN_GENERATING_OUTPUT
                = 4ull << 58
                #endif
                ,
            #else
            Int = 4ull << 29,
            #endif

            /**
             * 52-bit unsigned integer value.
             *
             * Set if @ref Json::parseUnsignedLongs() is called on a particular
             * subtree. Except for invalid values, parsing fails also fails if
             * any the values have a non-zero fractional part, if they have an
             * exponent, if they're negative or if they can't fit into 52 bits
             * (which is the representable unsigned integer range in a JSON).
             * @see @ref ParsedType::Size, @ref Json::parseSizes()
             */
            #ifndef CORRADE_TARGET_32BIT
            UnsignedLong
                #ifndef DOXYGEN_GENERATING_OUTPUT
                = 5ull << 58
                #endif
                ,
            #else
            UnsignedLong = 5ull << 29,
            #endif

            /**
             * 53-bit signed integer value.
             *
             * Set if @ref Json::parseLongs() is called on a particular
             * subtree. Except for invalid values, parsing fails also fails if
             * any the values have a non-zero fractional part, if they have an
             * exponent, if they're negative or if they can't fit into 53 bits
             * (which is the representable signed integer range in a JSON).
             *
             * Available only on 64-bit targets due to limits of the internal
             * representation. On @ref CORRADE_TARGET_32BIT "32-bit targets"
             * you can use either @ref ParsedType::Int, @ref ParsedType::Double
             * or parse the integer value always on-the-fly using
             * @ref parseLong().
             */
            #ifndef CORRADE_TARGET_32BIT
            Long
                #ifndef DOXYGEN_GENERATING_OUTPUT
                = 6ull << 58
                #endif
                ,
            #endif

            /**
             * Size value. Alias to @ref ParsedType::UnsignedInt or
             * @ref ParsedType::UnsignedLong depending on whether the system is
             * @ref CORRADE_TARGET_32BIT "32-bit" or 64-bit.
             * @see @ref Json::parseSizes()
             */
            Size
                #ifndef DOXYGEN_GENERATING_OUTPUT
                #ifndef CORRADE_TARGET_32BIT
                = UnsignedLong
                #else
                = UnsignedInt
                #endif
                #endif
                ,

            /** An object, array, null, bool or a string value. */
            #ifndef CORRADE_TARGET_32BIT
            Other
                #ifndef DOXYGEN_GENERATING_OUTPUT
                = 7ull << 58
                #endif
                ,
            #else
            Other = 7ull << 29,
            #endif
        };

        /**
         * @brief Token data
         *
         * Contains raw unparsed token data, including all child tokens (if
         * any). The first byte implies @ref type():
         *
         * -    `{` is a @ref Type::Object. Spans until and including the
         *      closing `}`. Child token tree is exposed through
         *      @ref children(). Immediate children are keys, second-level
         *      children are values.
         * -    `[` is a @ref Type::Array. Spans until and including the
         *      closing `]`. Child token tree is exposed through
         *      @ref children().
         * -    `n` is a @ref Type::Null. Not guaranteed to be a valid value if
         *      @ref isParsed() is not set.
         * -    `t` or `f` is a @ref Type::Bool. Not guaranteed to be a valid
         *      value if @ref isParsed() is not set.
         * -    `-` or `0` to `9` is a @ref Type::Number. Not guaranteed to be
         *      a valid value if @ref isParsed() is not set.
         * -    `"` is a @ref Type::String. If an object key, @ref children()
         *      contains the value token tree, but the token data always spans
         *      only until and including the closing `"`. Not guaranteed to be
         *      a valid value and may contain escape sequences if
         *      @ref isParsed() is not set.
         *
         * Returned view points to data owned by the originating @ref Json
         * instance, or to the string passed to @ref Json::fromString() if it
         * was called with @ref Containers::StringViewFlag::Global set. Due to
         * implementation complexity reasons, the global flag is not preserved
         * in the returned value here, only in case of @ref asString().
         */
        Containers::StringView data() const;

        /** @brief Token type */
        Type type() const;

        /**
         * @brief Whether the token value is parsed
         *
         * Set implicitly for @ref Type::Object and @ref Type::Array, for other
         * token types it means the value can be accessed directly by
         * @ref asNull(), @ref asBool(), @ref asDouble(), @ref asFloat(),
         * @ref asUnsignedInt(), @ref asInt(), @ref asUnsignedLong(),
         * @ref asLong(), @ref asSize() or @ref asString() function based on
         * @ref type() and @ref parsedType() and the call will not fail. If not
         * set, only @ref parseNull(), @ref parseBool(), @ref parseDouble(),
         * @ref parseFloat(), @ref parseUnsignedInt(), @ref parseInt(),
         * @ref parseUnsignedLong(), @ref parseLong(), @ref parseSize() or
         * @ref parseString() can be used.
         *
         * Tokens can be parsed during the initial run by passing
         * @ref Json::Option::ParseLiterals,
         * @relativeref{Json::Option,ParseDoubles},
         * @relativeref{Json::Option,ParseStringKeys} or
         * @relativeref{Json::Option,ParseStrings} to
         * @ref Json::fromString() or @ref Json::fromFile(), or selectively
         * later using @ref Json::parseLiterals(),
         * @relativeref{Json,parseDoubles()}, @relativeref{Json,parseFloats()},
         * @relativeref{Json,parseUnsignedInts()},
         * @relativeref{Json,parseInts()},
         * @relativeref{Json,parseUnsignedLongs()},
         * @relativeref{Json,parseLongs()}, @relativeref{Json,parseSizes()},
         * @relativeref{Json,parseStringKeys()} or
         * @relativeref{Json,parseStrings()}.
         */
        bool isParsed() const;

        /**
         * @brief Parsed token type
         *
         * @see @ref type(), @ref isParsed()
         */
        ParsedType parsedType() const;

        /**
         * @brief Child token count
         *
         * Number of all child tokens, including nested token trees. For
         * @ref Type::Null, @ref Type::Bool, @ref Type::Number and value
         * @ref Type::String always returns @cpp 0 @ce, for a @ref Type::String
         * that's an object key always returns @cpp 1 @ce.
         *
         * @m_class{m-note m-warning}
         *
         * @par
         *      The behavior is undefined if the function is called on a
         *      @ref JsonToken that has been copied out of the originating
         *      @ref Json instance.
         */
        std::size_t childCount() const;

        /**
         * @brief Child token tree
         *
         * Contains all child tokens ordered in a depth-first manner as
         * described in @ref Utility-Json-tokenization. Returned view points
         * to data owned by the originating @ref Json instance.
         *
         * @m_class{m-note m-warning}
         *
         * @par
         *      The behavior is undefined if the function is called on a
         *      @ref JsonToken that has been copied out of the originating
         *      @ref Json instance.
         *
         * @see @ref childCount(), @ref parent()
         */
        Containers::ArrayView<const JsonToken> children() const;

        /**
         * @brief First child token
         *
         * Returns first child token or @cpp nullptr @ce if there are no child
         * tokens. In particular, for a non-empty @ref Type::Object the first
         * immediate child is a @ref Type::String, which then contains the
         * value as a child token tree. @ref Type::Null, @ref Type::Bool and
         * @ref Type::Number tokens return @cpp nullptr @ce always. Accessing
         * the first child has a @f$ \mathcal{O}(1) @f$ complexity. Returned
         * value doints to data owned by the originating @ref Json instance.
         *
         * @m_class{m-note m-warning}
         *
         * @par
         *      The behavior is undefined if the function is called on a
         *      @ref JsonToken that has been copied out of the originating
         *      @ref Json instance.
         *
         * @see @ref parent(), @ref next()
         */
        const JsonToken* firstChild() const;

        /**
         * @brief Next token or next
         *
         * Return next token at the same or higher level, or a pointer to (one
         * value after) the end. Accessing the next token has a
         * @f$ \mathcal{O}(1) @f$ complexity. Returned value points to data
         * owned by the originating @ref Json instance.
         *
         * @m_class{m-note m-warning}
         *
         * @par
         *      The behavior is undefined if the function is called on a
         *      @ref JsonToken that has been copied out of the originating
         *      @ref Json instance.
         *
         * @see @ref parent()
         */
        const JsonToken* next() const {
            return this + childCount() + 1;
        }

        /**
         * @brief Parent token
         *
         * Returns parent token or @cpp nullptr @ce if the token is the root
         * token. Accessing the parent token is done by traversing the token
         * list backwards and thus has a @f$ \mathcal{O}(n) @f$ complexity ---
         * where possible, it's encouraged to remember the parent instead of
         * using this function. Returned value points to data owned by the
         * originating @ref Json instance.
         *
         * @m_class{m-note m-warning}
         *
         * @par
         *      The behavior is undefined if the function is called on a
         *      @ref JsonToken that has been copied out of the originating
         *      @ref Json instance.
         *
         * @see @ref firstChild(), @ref next(), @ref Json::root()
         */
        const JsonToken* parent() const;

        /**
         * @brief Get an iterable object
         *
         * Expects that the token is a @ref Type::Object, accessing
         * @ref JsonObjectItem::key() then expects that the key token has
         * @ref isParsed() set. See @ref Utility-Json-usage-iteration for more
         * information. Iteration through object keys is performed using
         * @ref next(), which has a @f$ \mathcal{O}(1) @f$ complexity.
         *
         * @m_class{m-note m-warning}
         *
         * @par
         *      The behavior is undefined if the function is called on a
         *      @ref JsonToken that has been copied out of the originating
         *      @ref Json instance.
         *
         * @see @ref type(), @ref Json::Option::ParseStringKeys,
         *      @ref Json::parseStringKeys(), @ref parseString()
         */
        JsonView<JsonObjectItem> asObject() const;

        /**
         * @brief Get an iterable array
         *
         * Expects that the token is a @ref Type::Array. See
         * @ref Utility-Json-usage-iteration for more information. Iteration
         * through array values is performed using @ref next(), which has a
         * @f$ \mathcal{O}(1) @f$ complexity.
         *
         * @m_class{m-note m-warning}
         *
         * @par
         *      The behavior is undefined if the function is called on a
         *      @ref JsonToken that has been copied out of the originating
         *      @ref Json instance.
         *
         * @see @ref type()
         */
        JsonView<JsonArrayItem> asArray() const;

        /**
         * @brief Find an object value by key
         *
         * Expects that the token is a @ref Type::Object and its keys have
         * @ref isParsed() set. If @p key is found, returns the child token
         * corresponding to its value, otherwise returns @cpp nullptr @ce.
         *
         * Note that there's no acceleration structure built at parse time and
         * thus the operation has a @f$ \mathcal{O}(n) @f$ complexity, where
         * @f$ n @f$ is the number of keys in given object. When looking up
         * many keys in a larger object, it's thus recommended to iterate
         * through @ref asObject() than to repeatedly call this function.
         *
         * @m_class{m-note m-warning}
         *
         * @par
         *      The behavior is undefined if the function is called on a
         *      @ref JsonToken that has been copied out of the originating
         *      @ref Json instance.
         *
         * @see @ref type(), @ref Json::Option::ParseStringKeys,
         *      @ref Json::parseStringKeys()
         */
        const JsonToken* find(Containers::StringView key) const;

        /**
         * @brief Find an array value by index
         *
         * Expects that the token is a @ref Type::Array. If @p index is found,
         * returns the corresponding token, otherwise returns @cpp nullptr @ce.
         *
         * Note that there's no acceleration structure built at parse time and
         * thus the operation has a @f$ \mathcal{O}(n) @f$ complexity, where
         * @f$ n @f$ is the number of items in given array. When looking up
         * many indices in a larger array, it's thus recommended to iterate
         * through @ref asArray() than to repeatedly call this function.
         *
         * @m_class{m-note m-warning}
         *
         * @par
         *      The behavior is undefined if the function is called on a
         *      @ref JsonToken that has been copied out of the originating
         *      @ref Json instance.
         *
         * @see @ref type()
         */
        const JsonToken* find(std::size_t index) const;

        /**
         * @brief Access an object value by key
         *
         * Compared to @ref find(Containers::StringView) const expects also
         * that @p key exists.
         */
        const JsonToken& operator[](Containers::StringView key) const;

        /**
         * @brief Access an array value by index
         *
         * Compared to @ref find(std::size_t) const expects also that @p index
         * exists.
         */
        const JsonToken& operator[](std::size_t index) const;

        /**
         * @brief Parse a null
         *
         * If the token is not a @ref Type::Null, returns
         * @ref Containers::NullOpt. If it is, but is not a valid value, prints
         * an error and returns @ref Containers::NullOpt. If @ref isParsed() is
         * already set, returns the cached value.
         */
        Containers::Optional<std::nullptr_t> parseNull() const;

        /**
         * @brief Get a parsed null value
         *
         * Expects that the token is @ref Type::Null and @ref isParsed() is
         * set. If not, use @ref parseNull() instead.
         * @see @ref Json::Option::ParseLiterals, @ref Json::parseLiterals()
         */
        std::nullptr_t asNull() const;

        /**
         * @brief Parse a boolean value
         *
         * If the token is not a @ref Type::Bool, returns
         * @ref Containers::NullOpt. If it is, but is not a valid value, prints
         * an error and returns @ref Containers::NullOpt. If @ref isParsed() is
         * already set, returns the cached value.
         * @see @ref asBool()
         */
        Containers::Optional<bool> parseBool() const;

        /**
         * @brief Get a parsed boolean value
         *
         * Expects that the token is @ref Type::Bool and @ref isParsed() is
         * set. If not, use @ref parseBool() instead.
         * @see @ref Json::Option::ParseLiterals, @ref Json::parseLiterals(),
         *      @ref asBoolArray()
         */
        bool asBool() const;

        /**
         * @brief Parse a 64-bit floating point value
         *
         * If the token is not a @ref Type::Number, returns
         * @ref Containers::NullOpt. If it is, but is not a valid value, prints
         * an error and returns @ref Containers::NullOpt. If the value is
         * already parsed as a @ref ParsedType::Double, returns the cached
         * value.
         * @see @ref asDouble()
         */
        Containers::Optional<double> parseDouble() const;

        /**
         * @brief Get a parsed 64-bit floating point value
         *
         * Expects that the token is already parsed as a
         * @ref ParsedType::Double. If not, use @ref parseDouble() instead.
         * @see @ref Json::Option::ParseDoubles, @ref Json::parseDoubles(),
         *      @ref asDoubleArray()
         */
        double asDouble() const;

        /**
         * @brief Parse a 32-bit floating point value
         *
         * If the token is not a @ref Type::Number, returns
         * @ref Containers::NullOpt. If it is, but is not a valid value, prints
         * an error and returns @ref Containers::NullOpt. Precision that
         * doesn't fit into the 32-bit floating point representation is
         * truncated, use @ref parseDouble() to get the full precision. If
         * the value is already parsed as a @ref ParsedType::Float, returns the
         * cached value.
         * @see @ref asFloat()
         */
        Containers::Optional<float> parseFloat() const;

        /**
         * @brief Get a parsed 32-bit floating point value
         *
         * Expects that the token is already parsed as a
         * @ref ParsedType::Float. If not, use @ref parseFloat() instead.
         * @see @ref Json::Option::ParseFloats, @ref Json::parseFloats(),
         *      @ref asFloatArray()
         */
        float asFloat() const;

        /**
         * @brief Parse an unsigned 32-bit integer value
         *
         * If the token is not a @ref Type::Number, returns
         * @ref Containers::NullOpt. If it is, but is not a valid value, has a
         * fractional or exponent part, is negative or doesn't fit into a
         * 32-bit representation, prints an error and returns
         * @ref Containers::NullOpt. If the value is already parsed as a
         * @ref ParsedType::UnsignedInt, returns the cached value.
         * @see @ref asUnsignedInt()
         */
        Containers::Optional<std::uint32_t> parseUnsignedInt() const;

        /**
         * @brief Get a parsed unsigned 32-bit integer value
         *
         * Expects that the token is already parsed as a
         * @ref ParsedType::UnsignedInt. If not, use @ref parseUnsignedInt()
         * instead.
         * @see @ref Json::parseUnsignedInts(), @ref asUnsignedIntArray()
         */
        std::uint32_t asUnsignedInt() const;

        /**
         * @brief Parse a signed 32-bit integer value
         *
         * If the token is not a @ref Type::Number, returns
         * @ref Containers::NullOpt, If it is, but is not a valid value, has a
         * fractional or exponent part or doesn't fit into a 32-bit
         * representation, prints an error and returns @ref Containers::NullOpt.
         * If the value is already parsed as a @ref ParsedType::Int, returns
         * the cached value.
         * @see @ref asInt()
         */
        Containers::Optional<std::int32_t> parseInt() const;

        /**
         * @brief Get a parsed signed 32-bit integer value
         *
         * Expects that the token is already parsed as a
         * @ref ParsedType::Int. If not, use @ref parseInt() instead.
         * @see @ref Json::parseInts(), @ref asIntArray()
         */
        std::int32_t asInt() const;

        /**
         * @brief Parse an unsigned 52-bit integer value
         *
         * If the token is not a @ref Type::Number, returns
         * @ref Containers::NullOpt. If it is, but is not a valid value, has a
         * fractional or exponent part, is negative or doesn't fit into 52 bits
         * (which is the representable unsigned integer range in a JSON),
         * prints an error and returns @ref Containers::NullOpt. If the value
         * is already parsed as a @ref ParsedType::UnsignedLong, returns the
         * cached value.
         * @see @ref asUnsignedLong()
         */
        Containers::Optional<std::uint64_t> parseUnsignedLong() const;

        /**
         * @brief Get a parsed unsigned 52-bit integer value
         *
         * Expects that the value is already parsed as a
         * @ref ParsedType::UnsignedLong. If not, use @ref parseUnsignedLong()
         * instead.
         * @see @ref Json::parseUnsignedLongs(), @ref asUnsignedLongArray()
         */
        std::uint64_t asUnsignedLong() const;

        /**
         * @brief Parse a signed 53-bit integer value
         *
         * If the token is not a @ref Type::Number, returns
         * @ref Containers::NullOpt. If it is, but is not a valid value, has a
         * fractional or exponent part or doesn't fit into 53 bits (which is
         * the representable signed integer range in a JSON), prints an error
         * and returns @ref Containers::NullOpt. If the value is already parsed
         * as a @ref ParsedType::Long, returns the cached value.
         * @see @ref asLong()
         */
        Containers::Optional<std::int64_t> parseLong() const;

        #ifndef CORRADE_TARGET_32BIT
        /**
         * @brief Get a parsed signed 53-bit integer value
         *
         * Expects that the token is already parsed as a
         * @ref ParsedType::Long. If not, use @ref parseLong() instead.
         *
         * Available only on 64-bit targets due to limits of the internal
         * representation. On @ref CORRADE_TARGET_32BIT "32-bit targets" you
         * can use either @ref ParsedType::Int, @ref ParsedType::Double or
         * parse the integer value always on-the-fly using @ref parseLong().
         * @see @ref Json::parseLongs(), @ref asLongArray()
         */
        std::int64_t asLong() const;
        #endif

        /**
         * @brief Parse a size value
         *
         * Convenience function that calls into @ref parseUnsignedInt() on
         * @ref CORRADE_TARGET_32BIT "32-bit targets" and into
         * @ref parseUnsignedLong() on 64-bit. Besides the concrete types as
         * documented in these functions, if the value is already parsed as a
         * @ref ParsedType::Size, returns the cached value.
         * @see @ref asSize()
         */
        Containers::Optional<std::size_t> parseSize() const;

        /**
         * @brief Get a parsed size value
         *
         * Expects that the value is already parsed as a
         * @ref ParsedType::Size. If not, use @ref parseSize() instead.
         */
        std::size_t asSize() const;

        /**
         * @brief Parse a string value
         *
         * If the token is not a @ref Type::String, returns
         * @ref Containers::NullOpt. If it is, but is not a valid value, prints
         * an error and returns @ref Containers::NullOpt.
         *
         * This function always returns a new copy of the string --- prefer
         * using @ref asString() if possible.
         */
        Containers::Optional<Containers::String> parseString() const;

        /**
         * @brief Get a parsed string value
         *
         * Expects that the token is a @ref Type::String and @ref isParsed() is
         * set. If @ref Json::fromString() was called with a global literal and
         * the string didn't contain any escape sequences, the returned view
         * has @ref Containers::StringViewFlag::Global set. If not, the view
         * points to data owned by the originating @ref Json instance.
         * @see @ref Json::Option::ParseStringKeys,
         *      @ref Json::Option::ParseStrings, @ref Json::parseStringKeys(),
         *      @ref Json::parseStrings(), @ref parseString()
         */
        Containers::StringView asString() const;

        /**
         * @brief Get a parsed boolean array
         *
         * Expects that the token is a @ref Type::Array. If the array is not
         * homogeneous parsed @ref Type::Bool, returns @ref Containers::NullOpt.
         * The returned view points to data owned by the originating @ref Json
         * instance.
         *
         * @m_class{m-note m-warning}
         *
         * @par
         *      The behavior is undefined if the function is called on a
         *      @ref JsonToken that has been copied out of the originating
         *      @ref Json instance.
         *
         * @see @ref type(), @ref Json::Option::ParseLiterals,
         *      @ref Json::parseLiterals(), @ref parseBool(), @ref asBool()
         */
        Containers::Optional<Containers::StridedArrayView1D<const bool>> asBoolArray() const;

        /**
         * @brief Get a parsed 64-bit floating point array
         *
         * Expects that the token is a @ref Type::Array. If the array is not
         * homogeneous @ref ParsedType::Double, returns
         * @ref Containers::NullOpt. The returned view points to data owned by
         * the originating @ref Json instance.
         *
         * @m_class{m-note m-warning}
         *
         * @par
         *      The behavior is undefined if the function is called on a
         *      @ref JsonToken that has been copied out of the originating
         *      @ref Json instance.
         *
         * @see @ref type(), @ref parsedType(),
         *      @ref Json::Option::ParseDoubles, @ref Json::parseDoubles(),
         *      @ref parseDouble(), @ref asDouble()
         */
        Containers::Optional<Containers::StridedArrayView1D<const double>> asDoubleArray() const;

        /**
         * @brief Get a parsed 32-bit floating point array
         *
         * Expects that the token is a @ref Type::Array. If the array is not
         * homogeneous @ref ParsedType::Float, returns
         * @ref Containers::NullOpt. The returned view points to data owned by
         * the originating @ref Json instance.
         *
         * @m_class{m-note m-warning}
         *
         * @par
         *      The behavior is undefined if the function is called on a
         *      @ref JsonToken that has been copied out of the originating
         *      @ref Json instance.
         *
         * @see @ref type(), @ref parsedType(), @ref Json::Option::ParseFloats,
         *      @ref Json::parseFloats(), @ref parseFloat(), @ref asFloat()
         */
        Containers::Optional<Containers::StridedArrayView1D<const float>> asFloatArray() const;

        /**
         * @brief Get a parsed unsigned 32-bit integer array
         *
         * Expects that the token is a @ref Type::Array. If the array is not
         * homogeneous @ref ParsedType::UnsignedInt, returns
         * @ref Containers::NullOpt. The returned view points to data owned by
         * the originating @ref Json instance.
         *
         * @m_class{m-note m-warning}
         *
         * @par
         *      The behavior is undefined if the function is called on a
         *      @ref JsonToken that has been copied out of the originating
         *      @ref Json instance.
         *
         * @see @ref type(), @ref parsedType(), @ref Json::parseUnsignedInts(),
         *      @ref parseUnsignedInt(), @ref asUnsignedInt()
         */
        Containers::Optional<Containers::StridedArrayView1D<const std::uint32_t>> asUnsignedIntArray() const;

        /**
         * @brief Get a parsed signed 32-bit integer array
         *
         * Expects that the token is a @ref Type::Array. If the array is not
         * homogeneous @ref ParsedType::Int, returns
         * @ref Containers::NullOpt. The returned view points to data owned by
         * the originating @ref Json instance.
         *
         * @m_class{m-note m-warning}
         *
         * @par
         *      The behavior is undefined if the function is called on a
         *      @ref JsonToken that has been copied out of the originating
         *      @ref Json instance.
         *
         * @see @ref type(), @ref parsedType(), @ref Json::parseInts(),
         *      @ref parseInt(), @ref asInt()
         */
        Containers::Optional<Containers::StridedArrayView1D<const std::int32_t>> asIntArray() const;

        /**
         * @brief Get a parsed unsigned 52-bit integer array
         *
         * Expects that the token is a @ref Type::Array. If the array is not
         * homogeneous @ref ParsedType::UnsignedLong, returns
         * @ref Containers::NullOpt. The returned view points to data owned by
         * the originating @ref Json instance.
         *
         * @m_class{m-note m-warning}
         *
         * @par
         *      The behavior is undefined if the function is called on a
         *      @ref JsonToken that has been copied out of the originating
         *      @ref Json instance.
         *
         * @see @ref type(), @ref parsedType(),
         *      @ref Json::parseUnsignedLongs(), @ref parseUnsignedLong(),
         *      @ref asUnsignedLong()
         */
        Containers::Optional<Containers::StridedArrayView1D<const std::uint64_t>> asUnsignedLongArray() const;

        #ifndef CORRADE_TARGET_32BIT
        /**
         * @brief Get a parsed signed 53-bit integer value array
         *
         * Expects that the token is a @ref Type::Array. If the array is not
         * homogeneous @ref ParsedType::Long, returns
         * @ref Containers::NullOpt.
         *
         * Available only on 64-bit targets due to limits of the internal
         * representation. On @ref CORRADE_TARGET_32BIT "32-bit targets" you
         * can use either @ref asIntArray, @ref asDoubleArray() or parse the
         * integer values one-by-one on-the-fly using @ref parseLong().
         *
         * @m_class{m-note m-warning}
         *
         * @par
         *      The behavior is undefined if the function is called on a
         *      @ref JsonToken that has been copied out of the originating
         *      @ref Json instance.
         *
         * @see @ref type(), @ref parsedType(), @ref Json::parseLongs(),
         *      @ref parseLong(), @ref asLong()
         */
        Containers::Optional<Containers::StridedArrayView1D<const std::int64_t>> asLongArray() const;
        #endif

        /**
         * @brief Get a parsed size array
         *
         * Convenience function that calls into @ref asUnsignedIntArray() on
         * @ref CORRADE_TARGET_32BIT "32-bit targets" and into
         * @ref asUnsignedLongArray() on 64-bit.
         *
         * @m_class{m-note m-warning}
         *
         * @par
         *      The behavior is undefined if the function is called on a
         *      @ref JsonToken that has been copied out of the originating
         *      @ref Json instance.
         *
         * @see @ref type(), @ref parsedType(), @ref Json::parseSizes(),
         *      @ref parseSize(), @ref asSize()
         */
        Containers::Optional<Containers::StridedArrayView1D<const std::size_t>> asSizeArray() const;

    private:
        friend Json;

        enum: std::uint64_t {
            #ifndef CORRADE_TARGET_32BIT
            /* Matching public Type, stored in last 3 bits of
               _sizeFlagsParsedTypeType */
            TypeMask = 0x07ull << 61, /* 0b111 */
            TypeObject = 1ull << 61,
            TypeArray = 2ull << 61,
            TypeNull = 3ull << 61,
            TypeBool = 4ull << 61,
            TypeNumber = 5ull << 61,
            TypeString = 6ull << 61,

            /* Matching public ParsedType, stored before the type in
               _sizeFlagsParsedTypeType */
            ParsedTypeMask = 0x07ull << 58, /* 0b111 */
            ParsedTypeNone = 0ull << 58,
            ParsedTypeDouble = 1ull << 58,
            ParsedTypeFloat = 2ull << 58,
            ParsedTypeUnsignedInt = 3ull << 58,
            ParsedTypeInt = 4ull << 58,
            ParsedTypeUnsignedLong = 5ull << 58,
            ParsedTypeLong = 6ull << 58,
            ParsedTypeOther = 7ull << 58,

            /* Stored before the parsed type in _sizeFlagsParsedTypeType */
            FlagStringKey = 1ull << 57,
            FlagStringGlobal = 1ull << 56,
            FlagStringEscaped = 1ull << 55,

            /* Size is the remaining 55 bits of _sizeFlagsParsedTypeType */
            SizeMask = (1ull << 55) - 1,
            #else
            NanMask = 0x7ffull << 52, /* 0b11111111111 */
            ChildCountMask = 0xffffffffull,

            /* Matching public Type, stored in _childCountFlagsTypeNan before
               NaN if NaN is set; if NaN is not set it's implicitly TypeNumber */
            TypeMask = 0x07ull << 49, /* 0b111 */
            TypeObject = 1ull << 49,
            TypeArray = 2ull << 49,
            TypeNull = 3ull << 49,
            TypeBool = 4ull << 49,
            TypeNumber = 5ull << 49,
            TypeString = 6ull << 49,

            /* Stored in _childCountFlagsTypeNan before the type if NaN is set;
               if NaN is not set the Parsed* values below are used instead */
            FlagParsed = 1ull << 48,
            FlagStringKey = 1ull << 47,
            FlagStringGlobal = 1ull << 46,
            FlagStringEscaped = 1ull << 45
            #endif
        };

        #ifdef CORRADE_TARGET_32BIT
        enum: std::uint32_t {
            /* Matching public ParsedType, stored in the last bits of
               _sizeParsedType if NaN is not set; if NaN is set the Flag*
               values above are used instead */
            ParsedTypeMask = 0x07u << 29, /* 0b111 */
            /* ParsedTypeNone does not apply here */
            ParsedTypeDouble = 1u << 29,
            ParsedTypeFloat = 2u << 29,
            ParsedTypeUnsignedInt = 3u << 29,
            ParsedTypeInt = 4u << 29,
            ParsedTypeUnsignedLong = 5u << 29,
            /* ParsedTypeOther does not apply here */

            /* If NaN is not set, size is the remaining 28 bits of
               _sizeParsedType */
            SizeMask = (1u << 28) - 1
        };
        #endif

        explicit JsonToken(NoInitT) /*nothing*/ {};
        constexpr explicit JsonToken(ValueInitT): _data{},
            #ifndef CORRADE_TARGET_32BIT
            _sizeFlagsParsedTypeType{},
            #else
            _sizeParsedType{},
            #endif
            #ifndef CORRADE_TARGET_32BIT
            _childCount{}
            #else
            _childCountFlagsTypeNan{}
            #endif
            {}

        /* See Json.cpp for detailed layout description and differences between
           32- and 64-bit representation */
        const char* _data;
        #ifndef CORRADE_TARGET_32BIT
        std::size_t _sizeFlagsParsedTypeType;
        #else
        std::size_t _sizeParsedType;
        #endif
        union {
            /* Child count abused for parent token index during parsing */
            #ifndef CORRADE_TARGET_32BIT
            std::uint64_t _childCount;
            #else
            std::uint64_t _childCountFlagsTypeNan;
            #endif
            /* Wouldn't the shorter types clash with NaN on BE? */
            bool _parsedBool;
            double _parsedDouble;
            float _parsedFloat;
            std::uint64_t _parsedUnsignedLong;
            #ifndef CORRADE_TARGET_32BIT
            std::int64_t _parsedLong;
            #endif
            std::uint32_t _parsedUnsignedInt;
            std::int32_t _parsedInt;
            Containers::String* _parsedString;
        };
};

/** @debugoperatorclassenum{JsonToken,Type} */
CORRADE_UTILITY_EXPORT Debug& operator<<(Debug& debug, JsonToken::Type value);

/** @debugoperatorclassenum{JsonToken,ParsedType} */
CORRADE_UTILITY_EXPORT Debug& operator<<(Debug& debug, JsonToken::ParsedType value);

/**
@brief JSON object item
@m_since_latest

Returned when iterating @ref JsonToken::asObject(). See
@ref Utility-Json-usage-iteration for more information.
*/
class CORRADE_UTILITY_EXPORT JsonObjectItem {
    public:
        /**
         * @brief Key
         *
         * Equivalent to calling @ref JsonToken::asString() on the token.
         */
        Containers::StringView key() const;

        /**
         * @brief Value
         *
         * Equvialent to accessing @ref JsonToken::firstChild() on the token.
         */
        const JsonToken& value() const {
            return *_token->firstChild();
        }

        /**
         * @brief Value
         *
         * Equvialent to calling @ref value().
         */
        /*implicit*/ operator const JsonToken&() const {
            return *_token->firstChild();
        }

    private:
        friend JsonIterator<JsonObjectItem>;

        /* The index is used only in JsonArrayItem, not here */
        explicit JsonObjectItem(std::size_t, const JsonToken& token) noexcept: _token{&token} {}

        const JsonToken* _token;
};

/**
@brief JSON array item
@m_since_latest

Returned when iterating @ref JsonToken::asObject(). See
@ref Utility-Json-usage-iteration for more information.
*/
class JsonArrayItem {
    public:
        /** @brief Array index */
        std::size_t index() const { return _index; }

        /** @brief Value */
        const JsonToken& value() const { return *_token; }

        /** @brief Value */
        operator const JsonToken&() const {
            return *_token;
        }

    private:
        friend JsonIterator<JsonArrayItem>;

        explicit JsonArrayItem(std::size_t index, const JsonToken& token) noexcept: _index{index}, _token{&token} {}

        std::size_t _index;
        const JsonToken* _token;
};

/**
@brief JSON iterator
@m_since_latest

Iterator for @ref JsonView, which is returned from @ref JsonToken::asObject()
and @ref JsonToken::asArray(). See @ref Utility-Json-usage-iteration for more
information.
*/
template<class T> class JsonIterator {
    public:
        /** @brief Equality comparison */
        bool operator==(const JsonIterator<T>& other) const {
            /* _index is implicit, no need to compare */
            return _token == other._token;
        }

        /** @brief Non-equality comparison */
        bool operator!=(const JsonIterator<T>& other) const {
            /* _index is implicit, no need to compare */
            return _token != other._token;
        }

        /**
         * @brief Advance to next position
         *
         * Implemented using @ref JsonToken::next().
         */
        JsonIterator<T>& operator++() {
            ++_index;
            _token = _token->next();
            return *this;
        }

        /** @brief Dereference */
        T operator*() const {
            return T{_index, *_token};
        }

    private:
        friend JsonView<T>;

        explicit JsonIterator(std::size_t index, const JsonToken* token) noexcept: _index{index}, _token{token} {}

        std::size_t _index;
        const JsonToken* _token;
};

/**
@brief JSON object and array view
@m_since_latest

Returned from @ref JsonToken::asObject() and @ref JsonToken::asArray(). See
@ref Utility-Json-usage-iteration for more information.
*/
template<class T> class JsonView {
    public:
        /** @brief Iterator to the first element */
        JsonIterator<T> begin() const { return JsonIterator<T>{0, _begin}; }
        /** @overload */
        JsonIterator<T> cbegin() const { return JsonIterator<T>{0, _begin}; }

        /** @brief Iterator to (one item after) the last element */
        JsonIterator<T> end() const { return JsonIterator<T>{0, _end}; }
        /** @overload */
        JsonIterator<T> cend() const { return JsonIterator<T>{0, _end}; }

    private:
        friend JsonToken;

        explicit JsonView(const JsonToken* begin, const JsonToken* end) noexcept: _begin{begin}, _end{end} {}

        const JsonToken* _begin;
        const JsonToken* _end;
};

inline JsonToken::Type JsonToken::type() const {
    #ifndef CORRADE_TARGET_32BIT
    return Type(_sizeFlagsParsedTypeType & TypeMask);
    #else
    /* If NaN is set, the type is stored */
    if((_childCountFlagsTypeNan & NanMask) == NanMask)
        return Type(_childCountFlagsTypeNan & TypeMask);
    /* Otherwise it's implicitly a number */
    return Type::Number;
    #endif
}

inline bool JsonToken::isParsed() const {
    #ifndef CORRADE_TARGET_32BIT
    return _sizeFlagsParsedTypeType & ParsedTypeMask;
    #else
    /* If NaN is set, it's parsed if any bit of the parsed type is set */
    if((_childCountFlagsTypeNan & NanMask) == NanMask)
        return _childCountFlagsTypeNan & FlagParsed;
    /* Otherwise it's an already parsed number */
    return true;
    #endif
}

inline JsonToken::ParsedType JsonToken::parsedType() const {
    #ifndef CORRADE_TARGET_32BIT
    return ParsedType(_sizeFlagsParsedTypeType & ParsedTypeMask);
    #else
    /* If NaN is set, the parsed type is either None or Other */
    if((_childCountFlagsTypeNan & NanMask) == NanMask)
        return _childCountFlagsTypeNan & FlagParsed ?
            ParsedType::Other : ParsedType::None;
    /* Otherwise it's a number and the parsed type is stored in size */
    return ParsedType(_sizeParsedType & ParsedTypeMask);
    #endif
}

inline const JsonToken* JsonToken::firstChild() const {
    #ifndef CORRADE_TARGET_32BIT
    /* The token has a child if it's an object or an array and has children */
    if((((_sizeFlagsParsedTypeType & TypeMask) == TypeObject ||
         (_sizeFlagsParsedTypeType & TypeMask) == TypeArray) && _childCount) ||
        /* or if it's an object key */
        (_sizeFlagsParsedTypeType & FlagStringKey))
        return this + 1;
    #else
    /* The token has a child if it's not a parsed number and */
    if(((_childCountFlagsTypeNan & NanMask) == NanMask) &&
      /* it's an object with non-zero child count */
    ((((_childCountFlagsTypeNan & TypeMask) == TypeObject ||
       (_childCountFlagsTypeNan & TypeMask) == TypeArray) &&
       (_childCountFlagsTypeNan & ChildCountMask)) ||
       /* or it's an object key */
       (_childCountFlagsTypeNan & FlagStringKey)))
        return this + 1;
    #endif
    return nullptr;
}

inline std::nullptr_t JsonToken::asNull() const {
    CORRADE_ASSERT(type() == Type::Null && isParsed(),
        "Utility::JsonToken::asNull(): token is" << (isParsed() ? "a parsed" : "an unparsed") << type(), {});
    return nullptr;
}

inline bool JsonToken::asBool() const {
    CORRADE_ASSERT(type() == Type::Bool && isParsed(),
        "Utility::JsonToken::asBool(): token is" << (isParsed() ? "a parsed" : "an unparsed") << type(), {});
    return _parsedBool;
}

inline double JsonToken::asDouble() const {
    CORRADE_ASSERT(parsedType() == ParsedType::Double,
        "Utility::JsonToken::asDouble(): token is a" << type() << "parsed as" << parsedType(), {});
    return _parsedDouble;
}

inline float JsonToken::asFloat() const {
    CORRADE_ASSERT(parsedType() == ParsedType::Float,
        "Utility::JsonToken::asFloat(): token is a" << type() << "parsed as" << parsedType(), {});
    return _parsedFloat;
}

inline std::uint32_t JsonToken::asUnsignedInt() const {
    CORRADE_ASSERT(parsedType() == ParsedType::UnsignedInt,
        "Utility::JsonToken::asUnsignedInt(): token is a" << type() << "parsed as" << parsedType(), {});
    return _parsedUnsignedInt;
}

inline std::int32_t JsonToken::asInt() const {
    CORRADE_ASSERT(parsedType() == ParsedType::Int,
        "Utility::JsonToken::asInt(): token is a" << type() << "parsed as" << parsedType(), {});
    return _parsedInt;
}

inline std::uint64_t JsonToken::asUnsignedLong() const {
    CORRADE_ASSERT(parsedType() == ParsedType::UnsignedLong,
        "Utility::JsonToken::asUnsignedLong(): token is a" << type() << "parsed as" << parsedType(), {});
    return _parsedUnsignedLong;
}

#ifndef CORRADE_TARGET_32BIT
inline std::int64_t JsonToken::asLong() const {
    CORRADE_ASSERT(parsedType() == ParsedType::Long,
        "Utility::JsonToken::asLong(): token is a" << type() << "parsed as" << parsedType(), {});
    return _parsedLong;
}
#endif

inline std::size_t JsonToken::asSize() const {
    CORRADE_ASSERT(parsedType() == ParsedType::Size,
        "Utility::JsonToken::asSize(): token is a" << type() << "parsed as" << parsedType(), {});
    #ifndef CORRADE_TARGET_32BIT
    return _parsedUnsignedLong;
    #else
    return _parsedUnsignedInt;
    #endif
}

}}

#endif