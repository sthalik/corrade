#pragma once

#include "Corrade/Utility/Macros.h"
#include "Corrade/Utility/Utility.h"
#include "Corrade/Utility/visibility.h"
#include <cstddef>
#include <type_traits>

namespace Corrade { namespace Utility { namespace Implementation { namespace MyAssert {

[[noreturn]] CORRADE_UTILITY_EXPORT CORRADE_NEVER_INLINE void emit_abort() noexcept;
[[noreturn]] CORRADE_UTILITY_EXPORT CORRADE_NEVER_INLINE void emit_assert_fail(const char* message) noexcept;
CORRADE_UTILITY_EXPORT CORRADE_NEVER_INLINE Utility::Error& emit_message_begin() noexcept;
[[noreturn]] CORRADE_UTILITY_EXPORT CORRADE_NEVER_INLINE void emit_message_end() noexcept;

template<typename T> T assert_expression(T&& value, const char* message)
{
    if CORRADE_UNLIKELY(static_cast<bool>(!value))
        emit_assert_fail(message);
    return static_cast<T&&>(value);
}

struct message_begin {};

// Small trivially copyable operands by value: a reference takes the operand's address, which
// keeps it out of registers and blocks vectorization.
template<typename T, bool = std::is_trivially_copyable<T>::value> struct message_small : std::false_type {};
template<typename T> struct message_small<T, true> : std::integral_constant<bool, sizeof(T) <= 2*sizeof(void*)> {};

template<typename T> struct message_by_value : std::integral_constant<bool,
    !std::is_array<T>::value && (std::is_scalar<T>::value || std::is_function<T>::value || message_small<T>::value)> {};

template<typename T, bool = message_by_value<T>::value> struct message_value { using type = const T&; };
template<typename T> struct message_value<T, true> { using type = typename std::decay<T>::type; };
template<typename T, std::size_t N> struct message_value<T[N], false> { using type = const T*; };

template<typename Prev, typename V> struct message_item
{
    const Prev& prev;
    V value;
};

template<typename T> message_item<message_begin, typename message_value<T>::type> operator<<(const message_begin& prev, const T& value) { return {prev, value}; }
template<typename P, typename V, typename T> message_item<message_item<P, V>, typename message_value<T>::type> operator<<(const message_item<P, V>& prev, const T& value) { return {prev, value}; }

template<typename... Vs> [[noreturn]] CORRADE_NEVER_INLINE void emit_message_fail(Vs... values) noexcept
{
    Utility::Error& error = emit_message_begin();
    const int expand[]{0, ((void)(error << values), 0)...};
    (void)expand;
    emit_message_end();
}

// Passes the operands as arguments, so the message_item chain never reaches memory and adds
// nothing to the caller's inline cost beyond the call.
template<typename... Vs> struct message_unpack
{
    [[noreturn]] static CORRADE_ALWAYS_INLINE void fail(const message_begin&, Vs... values) noexcept
    {
        emit_message_fail<Vs...>(values...);
    }
    template<typename P, typename V> [[noreturn]] static CORRADE_ALWAYS_INLINE void fail(const message_item<P, V>& item, Vs... values) noexcept
    {
        message_unpack<V, Vs...>::fail(item.prev, item.value, values...);
    }
};

template<typename M> [[noreturn]] CORRADE_ALWAYS_INLINE void message_fail(const M& message) noexcept
{
    message_unpack<>::fail(message);
}

}}}} // namespace Corrade::Utility::Implementation::MyAssert

// Graceful asserts and user-defined abort macros need Assert.h's own macros. Corrade's tests use both.
#if !defined CORRADE_NO_ASSERT && !defined CORRADE_STANDARD_ASSERT && !defined CORRADE_GRACEFUL_ASSERT && \
    !defined CORRADE_ASSERT_INCLUDE && !defined CORRADE_ASSERT_ABORT && !defined CORRADE_ASSERT_MESSAGE_ABORT

#define CORRADE_MYASSERT_FAIL(message) ::Corrade::Utility::Implementation::MyAssert::emit_assert_fail(message)
#define CORRADE_MYASSERT_MESSAGE_FAIL(...) \
    ::Corrade::Utility::Implementation::MyAssert::message_fail(::Corrade::Utility::Implementation::MyAssert::message_begin{} << __VA_ARGS__)

#define CORRADE_ASSERT_ABORT() ::Corrade::Utility::Implementation::MyAssert::emit_abort()
#define CORRADE_ASSERT_MESSAGE_ABORT(...) CORRADE_MYASSERT_MESSAGE_FAIL(__VA_ARGS__);

// Each condition is tested with the same expression as in Assert.h. Where operator! is not the
// negation of operator bool, as in Magnum's BitVector, `x`, `!!x` and `!x` disagree.
#define CORRADE_ASSERT(condition, message, returnValue) \
    (CORRADE_INTERNAL_EXPECT_1(static_cast<bool>(!!(condition))) ? void() : CORRADE_MYASSERT_MESSAGE_FAIL(message))
#define CORRADE_CONSTEXPR_ASSERT(condition, message) \
    (CORRADE_INTERNAL_EXPECT_1(static_cast<bool>(!!(condition))) ? void() : CORRADE_MYASSERT_MESSAGE_FAIL(message))
#define CORRADE_ASSERT_OUTPUT(call, message, returnValue) \
    (CORRADE_INTERNAL_EXPECT_0(static_cast<bool>(!(call))) ? CORRADE_MYASSERT_MESSAGE_FAIL(message) : void())
#define CORRADE_ASSERT_UNREACHABLE(message, returnValue) CORRADE_MYASSERT_MESSAGE_FAIL(message)

#define CORRADE_INTERNAL_ASSERT(condition) \
    (CORRADE_INTERNAL_EXPECT_0(static_cast<bool>(!(condition))) ? CORRADE_MYASSERT_FAIL("Assertion " #condition " failed at " __FILE__ ":" CORRADE_LINE_STRING) : void())
#define CORRADE_INTERNAL_CONSTEXPR_ASSERT(condition) \
    (CORRADE_INTERNAL_EXPECT_1(static_cast<bool>(condition)) ? void() : CORRADE_MYASSERT_FAIL("Assertion " #condition " failed at " __FILE__ ":" CORRADE_LINE_STRING))
#define CORRADE_INTERNAL_ASSERT_OUTPUT(call) \
    (CORRADE_INTERNAL_EXPECT_0(static_cast<bool>(!(call))) ? CORRADE_MYASSERT_FAIL("Assertion " #call " failed at " __FILE__ ":" CORRADE_LINE_STRING) : void())
#define CORRADE_INTERNAL_ASSERT_EXPRESSION(...) \
    ::Corrade::Utility::Implementation::MyAssert::assert_expression(__VA_ARGS__, "Assertion " #__VA_ARGS__ " failed at " __FILE__ ":" CORRADE_LINE_STRING)
#define CORRADE_INTERNAL_ASSERT_UNREACHABLE() CORRADE_MYASSERT_FAIL("Reached unreachable code at " __FILE__ ":" CORRADE_LINE_STRING)

#include "Corrade/Utility/Debug.h"

#endif
