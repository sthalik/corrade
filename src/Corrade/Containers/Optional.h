#ifndef Corrade_Containers_Optional_h
#define Corrade_Containers_Optional_h
/*
    This file is part of Corrade.

    Copyright © 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016,
                2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025, 2026
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
 * @brief Class @ref Corrade::Containers::Optional, tag type @ref Corrade::Containers::NullOptT, tag @ref Corrade::Containers::NullOpt, function @ref Corrade::Containers::optional()
 * @see @ref Corrade/Containers/OptionalStl.h
 */

#include <new>
/* std::declval() is said to be in <utility> but libstdc++, libc++ and MSVC STL
   all have it directly in <type_traits> because it just makes sense */
#include <type_traits>

#include "Corrade/Tags.h"
#include "Corrade/Containers/constructHelpers.h"
#include "Corrade/Utility/DebugAssert.h"
#include "Corrade/Utility/Macros.h"
#ifndef CORRADE_SINGLES_NO_DEBUG
#include "Corrade/Utility/Debug.h"
#endif

#ifndef CORRADE_CONTAINERS_NO_SANITIZER_ANNOTATIONS
#ifdef __has_feature
#if __has_feature(address_sanitizer)
#define _CORRADE_CONTAINERS_OPTIONAL_SANITIZER_ENABLED
#endif
#endif
#ifdef __SANITIZE_ADDRESS__
#define _CORRADE_CONTAINERS_OPTIONAL_SANITIZER_ENABLED
#endif
#endif

#ifdef _CORRADE_CONTAINERS_OPTIONAL_SANITIZER_ENABLED
#include <cstddef>
extern "C" void __asan_poison_memory_region(void const volatile* addr, std::size_t size);
extern "C" void __asan_unpoison_memory_region(void const volatile* addr, std::size_t size);
#endif

namespace Corrade { namespace Containers {

namespace Implementation {
    template<class, class> struct OptionalConverter;

    /* Copy and move members disabled for T take this, which nothing converts
       to, not even {}. The copy members the compiler then declares are deleted. */
    class NotAType {
        NotAType(const NotAType&);
    };
}

/**
@brief Null optional initialization tag type

Used to make initialization of null @ref Optional more explicit.
*/
/* Explicit constructor to avoid ambiguous calls when using {} */
struct NullOptT {
    #ifndef DOXYGEN_GENERATING_OUTPUT
    struct Init{};
    constexpr explicit NullOptT(Init) {}
    #endif
};

/**
@brief Null optional initialization tag

Use for explicit initialization of null @ref Optional. Checking an optional for
emptiness can be done either by comparing it against this variable or by making
use of its (explicit) boolean conversion.
*/
constexpr NullOptT NullOpt{NullOptT::Init{}};

/**
@brief Lightweight optional value

Equivalent to @ref std::optional from C++17, provides an optional checked
storage for object of type @p T. The optional object can be seen as a container
of @p T objects with maximal size 1 and can be in two states, either empty or
having a value. A non-allocating counterpart to @ref Pointer.

A common use for an optional object is for a return value of function that can
fail --- like @ref Pointer, but without the unnecessary allocation overhead.
Similarly to @ref Pointer, the presence of an object can be checked using
@ref operator bool(), or alternatively by comparing to @ref NullOpt (same as a
pointer can be compared to @cpp nullptr @ce). The stored object can be accessed
using @ref operator->(), @ref operator*() or using implicit conversion, while
attempt to access a stored object in an empty state leads to assertion error.

Unlike @ref std::optional, this class does not provide ordering operators, and
only the reference specialization is @cpp constexpr @ce, which makes it fairly
simple and lightweight. If you need the extra features, use the standard
@ref std::optional.

An @cpp Optional<const T> @ce can be emplaced and reset to @ref NullOpt, but
not assigned from another optional, same as @ref std::optional.

@section Containers-Optional-references Optional references

An @cpp Optional<T&> @ce stores a pointer that's @cpp nullptr @ce when empty.
It doesn't bind to temporaries, assignment and @ref emplace() rebind it, and
it's implicitly constructible from an @cpp Optional<U&> @ce or a reference to
an @cpp Optional<U> @ce if a @cpp U* @ce is convertible to @cpp T* @ce. The
latter refers to the value of the @cpp Optional<U> @ce, or is empty if it's
empty. Optional rvalue references are not allowed.

The @ref optional(T&&) overload is picked even for @cpp optional<int&>(a) @ce
and returns an @cpp Optional<int> @ce with a copy of @cpp a @ce. Construct an
@cpp Optional<int&> @ce directly instead.

@section Containers-Optional-stl STL compatibility

Instances of @ref Optional are *explicitly* copy- and move-convertible to and
from @ref std::optional if you include @ref Corrade/Containers/OptionalStl.h
and build your code with C++17 enabled. The conversion is provided in a
separate header to avoid unconditional @cpp #include <optional> @ce, which
significantly affects compile times. Additionally, the @ref optional(T&&)
overload also allows for such a conversion. Example:

@snippet Containers-stl17.cpp Optional

<b></b>

@m_class{m-block m-success}

@par Single-header version
    This class is also available as a single-header, dependency-less
    [CorradeOptional.h](https://github.com/mosra/magnum-singles/tree/master/CorradeOptional.h)
    library in the Magnum Singles repository for easier integration into your
    projects. See @ref corrade-singles for more information. The above
    mentioned STL compatibility is included as well, but disabled by default.
    Enable it by compiling as C++17 and specifying
    @cpp #define CORRADE_OPTIONAL_STL_COMPATIBILITY @ce before including the
    file. Including it multiple times with different macros defined works as
    well.

@see @ref NullOpt, @ref optional(T&&), @ref optional(Args&&... args),
    @ref Reference, @ref Array1
*/
template<class T> class Optional {
    public:
        /**
         * @brief Value type
         * @m_since_latest
         */
        typedef T Type;

        /**
         * @brief Default constructor
         *
         * Creates an optional object in empty state.
         * @see @ref operator bool(), @ref emplace()
         */
        /*implicit*/ Optional(NullOptT = NullOpt) noexcept: _set{false} {
            poisonValue();
        }

        /**
         * @brief Construct optional object by copy
         *
         * Stores a copy of passed object.
         * @see @ref operator bool(), @ref operator->(), @ref operator*()
         */
        /*implicit*/ Optional(const T& value) noexcept(std::is_nothrow_copy_constructible<T>::value): _set{true} {
            /* Can't use {}, see the GCC 4.8-specific overload for details */
            #if defined(CORRADE_TARGET_GCC) && !defined(CORRADE_TARGET_CLANG) && __GNUC__ < 5
            Implementation::construct(_value, value);
            #else
            new(&_value) typename std::remove_const<T>::type{value};
            #endif
        }

        /**
         * @brief Construct optional object by move
         *
         * Moves the passed object to internal storage.
         * @see @ref operator bool(), @ref operator->(), @ref operator*()
         */
        #ifdef DOXYGEN_GENERATING_OUTPUT
        /*implicit*/ Optional(T&& value) noexcept(std::is_nothrow_move_constructible<T>::value);
        #else
        /* Not T&&, from which a const T could only be copied */
        /*implicit*/ Optional(typename std::remove_const<T>::type&& value) noexcept(std::is_nothrow_move_constructible<typename std::remove_const<T>::type>::value): _set{true} {
            /* Can't use {}, see the GCC 4.8-specific overload for details */
            #if defined(CORRADE_TARGET_GCC) && !defined(CORRADE_TARGET_CLANG) && __GNUC__ < 5
            Implementation::construct(_value, Utility::move(value));
            #else
            new(&_value) typename std::remove_const<T>::type{Utility::move(value)};
            #endif
        }
        #endif

        /**
         * @brief Construct optional object in-place
         *
         * Constructs the value by passing @p args to its constructor in-place.
         * @see @ref operator bool(), @ref operator->(), @ref operator*(),
         *      @ref emplace()
         */
        template<class ...Args> /*implicit*/ Optional(Corrade::InPlaceInitT, Args&&... args) noexcept(std::is_nothrow_constructible<T, Args&&...>::value): _set{true} {
            Implementation::construct(_value, Utility::forward<Args>(args)...);
        }

        /**
         * @brief Copy-construct an optional from external representation
         *
         * @see @ref Containers-Optional-stl, @ref optional(T&&)
         */
        template<class U, class = decltype(Implementation::OptionalConverter<T, U>::from(std::declval<const U&>()))> explicit Optional(const U& other) noexcept(std::is_nothrow_copy_constructible<T>::value): Optional{Implementation::OptionalConverter<T, U>::from(other)} {}

        /**
         * @brief Move-construct an optional from external representation
         *
         * @see @ref Containers-Optional-stl, @ref optional(T&&)
         */
        template<class U, class = decltype(Implementation::OptionalConverter<T, U>::from(std::declval<U&&>()))> explicit Optional(U&& other) noexcept(std::is_nothrow_move_constructible<T>::value): Optional{Implementation::OptionalConverter<T, U>::from(Utility::move(other))} {}

        #ifdef DOXYGEN_GENERATING_OUTPUT
        /**
         * @brief Copy constructor
         *
         * Deleted if @p T isn't copy-constructible.
         */
        Optional(const Optional<T>& other) noexcept(std::is_nothrow_copy_constructible<T>::value);

        /**
         * @brief Move constructor
         *
         * Deleted if @p T isn't move-constructible. A @cpp const @ce value
         * is copied.
         */
        Optional(Optional<T>&& other) noexcept(std::is_nothrow_move_constructible<T>::value);

        /**
         * @brief Copy assignment
         *
         * If the object already contains a value, calls its destructor.
         * Copy-constructs the value from @p other using placement-new.
         * Deleted if @p T isn't copy-constructible or is @cpp const @ce.
         */
        Optional<T>& operator=(const Optional<T>& other) noexcept(std::is_nothrow_copy_assignable<T>::value);

        /**
         * @brief Move assignment
         *
         * If both objects contain a value, the value is swapped. Otherwise,
         * if the object contains a value, calls its destructor. If @p other
         * contains a value, move-constructs the value from it using placement
         * new. Deleted if @p T isn't move-constructible or is
         * @cpp const @ce.
         */
        Optional<T>& operator=(Optional<T>&& other) noexcept(std::is_nothrow_move_assignable<T>::value);
        #else
        Optional(typename std::conditional<std::is_copy_constructible<T>::value, const Optional<T>&, const Implementation::NotAType&>::type other) noexcept(std::is_nothrow_copy_constructible<T>::value): _set(other._set) {
            if(_set)
                /* Can't use {}, see the GCC 4.8-specific overload for details */
                #if defined(CORRADE_TARGET_GCC) && !defined(CORRADE_TARGET_CLANG) && __GNUC__ < 5
                Implementation::construct(_value, other._value);
                #else
                new(&_value) typename std::remove_const<T>::type{other._value};
                #endif
            else
                poisonValue();
        }

        Optional(typename std::conditional<std::is_move_constructible<T>::value, Optional<T>&&, Implementation::NotAType&&>::type other) noexcept(std::is_nothrow_move_constructible<T>::value): _set(other._set) {
            if(_set)
                /* Can't use {}, see the GCC 4.8-specific overload for details */
                #if defined(CORRADE_TARGET_GCC) && !defined(CORRADE_TARGET_CLANG) && __GNUC__ < 5
                Implementation::construct(_value, Utility::move(static_cast<T&>(other._value)));
                #else
                new(&_value) typename std::remove_const<T>::type{Utility::move(static_cast<T&>(other._value))};
                #endif
            else
                poisonValue();
        }

        Optional<T>& operator=(typename std::conditional<std::is_copy_constructible<T>::value && !std::is_const<T>::value, const Optional<T>&, const Implementation::NotAType&>::type other) noexcept(std::is_nothrow_copy_assignable<T>::value) {
            if(_set)
                _value.~T();
            else
                unpoisonValue();
            if((_set = other._set))
                /* Can't use {}, see the GCC 4.8-specific overload for details */
                #if defined(CORRADE_TARGET_GCC) && !defined(CORRADE_TARGET_CLANG) && __GNUC__ < 5
                Implementation::construct(_value, other._value);
                #else
                new(&_value) T{other._value};
                #endif
            else
                poisonValue();
            return *this;
        }

        Optional<T>& operator=(typename std::conditional<std::is_move_constructible<T>::value && !std::is_const<T>::value, Optional<T>&&, Implementation::NotAType&&>::type other) noexcept(std::is_nothrow_move_assignable<T>::value) {
            if(_set && other._set) {
                using Utility::swap;
                swap(other._value, _value);
            } else {
                if(_set)
                    _value.~T();
                else
                    unpoisonValue();
                if((_set = other._set))
                    /* Can't use {}, see the GCC 4.8-specific overload for
                       details */
                    #if defined(CORRADE_TARGET_GCC) && !defined(CORRADE_TARGET_CLANG) && __GNUC__ < 5
                    Implementation::construct(_value, Utility::move(other._value));
                    #else
                    new(&_value) T{Utility::move(other._value)};
                    #endif
                else
                    poisonValue();
            }
            return *this;
        }
        #endif

        /**
         * @brief Copy-convert the optional to external representation
         *
         * @see @ref Containers-Optional-stl
         */
        template<class U, class = decltype(Implementation::OptionalConverter<T, U>::to(std::declval<const Optional<T>&>()))> explicit operator U() const & {
            return Implementation::OptionalConverter<T, U>::to(*this);
        }

        /**
         * @brief Move-convert the optional to external representation
         *
         * @see @ref Containers-Optional-stl
         */
        template<class U, class = decltype(Implementation::OptionalConverter<T, U>::to(std::declval<Optional<T>&&>()))> explicit operator U() && {
            return Implementation::OptionalConverter<T, U>::to(Utility::move(*this));
        }

        /**
         * @brief Clear the contained value
         *
         * If the object contains a value, calls its destructor. Compared to
         * @ref operator=(Optional<T>&&) this doesn't require the type to be
         * move-assignable.
         */
        Optional<T>& operator=(NullOptT) noexcept;

        /**
         * @brief Destructor
         *
         * If the optional object is not empty, calls destructor on stored
         * value.
         */
        ~Optional() {
            if(_set)
                _value.~T();
            else
                /* The owner may reuse the storage for anything */
                unpoisonValue();
        }

        /**
         * @brief Whether the optional object has a value
         *
         * Returns @cpp true @ce if the optional object has a value,
         * @cpp false @ce otherwise.
         */
        explicit operator bool() const { return _set; }

        /**
         * @brief Equality comparison to another optional
         *
         * Returns @cpp true @ce if either both instances are empty or both
         * instances have a value and the values compare equal, @cpp false @ce
         * otherwise.
         */
        bool operator==(const Optional<T>& other) const {
            return (!_set && !other._set) || (_set && other._set && _value == other._value);
        }

        /**
         * @brief Non-equality comparison to another optional
         *
         * Returns negation of @ref operator==(const Optional<T>&) const.
         */
        bool operator!=(const Optional<T>& other) const { return !operator==(other); }

        /**
         * @brief Equality comparison to a null optional
         *
         * Returns @cpp true @ce if the instance is empty, @cpp false @ce
         * otherwise.
         * @see @ref operator bool()
         */
        bool operator==(NullOptT) const { return !_set; }

        /**
         * @brief Non-equality comparison to a null optional
         *
         * Returns @cpp true @ce if the instance has a value, @cpp false @ce
         * otherwise.
         * @see @ref operator bool()
         */
        bool operator!=(NullOptT) const { return _set; }

        /**
         * @brief Equality comparison to a value
         *
         * Returns @cpp true @ce if the instance has a value which compares
         * equal to @p other, @cpp false @ce otherwise.
         */
        bool operator==(const T& other) const {
            return _set ? _value == other : false;
        }

        /**
         * @brief Non-equality comparison to a value
         *
         * Returns negation of @ref operator==(const T&) const.
         */
        bool operator!=(const T& other) const { return !operator==(other); }

        /**
         * @brief Access the stored object
         *
         * Expects that the optional object has a value.
         * @see @ref operator bool(), @ref operator*()
         */
        T* operator->() {
            CORRADE_DEBUG_ASSERT(_set, "Containers::Optional: the optional is empty", &_value);
            return &_value;
        }

        /** @overload */
        const T* operator->() const {
            CORRADE_DEBUG_ASSERT(_set, "Containers::Optional: the optional is empty", &_value);
            return &_value;
        }

        /**
         * @brief Access the stored object
         *
         * Expects that the optional object has a value.
         * @see @ref operator bool(), @ref operator->()
         */
        T& operator*() & {
            CORRADE_DEBUG_ASSERT(_set, "Containers::Optional: the optional is empty", _value);
            return _value;
        }

        /** @overload */
        /* Not T&& because that'd cause nasty dangling reference issues in
           common code. See the accessRvalueLifetimeExtension() test for
           details. */
        typename std::remove_const<T>::type operator*() && {
            CORRADE_DEBUG_ASSERT(_set, "Containers::Optional: the optional is empty", Utility::move(static_cast<T&>(_value)));
            return Utility::move(static_cast<T&>(_value));
        }

        /** @overload */
        const T& operator*() const & {
            CORRADE_DEBUG_ASSERT(_set, "Containers::Optional: the optional is empty", _value);
            return _value;
        }

        /* No const&& overload right now. There's one theoretical use case,
           where an API could return a `const Optional<T>`, and then if there
           would be a `operator*() const&&` overload returning a `T` (and not
           `T&&`), it could get picked over the `const T&`, solving the same
           problem as the `operator*() &&` above. At the moment I don't see a
           practical reason to return a const value, so this isn't handled. */

        /**
         * @brief Emplace a new value
         *
         * If the object already contains a value, calls its destructor.
         * Constructs the value by passing @p args to its constructor using
         * placement new.
         */
        template<class ...Args> T& emplace(Args&&... args);

    private:
        template<class> friend class Optional;

        /* An empty optional's storage is poisoned if it's at least 8 bytes.
           ASan can poison only the tail of an 8-byte granule, so if _set
           shares the value's last granule, that granule stays readable. */
        #ifdef _CORRADE_CONTAINERS_OPTIONAL_SANITIZER_ENABLED
        CORRADE_ALWAYS_INLINE void poisonValue() noexcept {
            if(sizeof(T) >= 8)
                __asan_poison_memory_region(&_value, sizeof(T));
        }
        CORRADE_ALWAYS_INLINE void unpoisonValue() noexcept {
            if(sizeof(T) >= 8)
                __asan_unpoison_memory_region(&_value, sizeof(T));
        }
        #else
        CORRADE_ALWAYS_INLINE void poisonValue() noexcept {}
        CORRADE_ALWAYS_INLINE void unpoisonValue() noexcept {}
        #endif

        union {
            typename std::remove_const<T>::type _value;
            /* GCC 4.8, 7, 9, 10, 11, 12, 13 in Release produces a "warning:
               Optional<T>::<unnamed>.Optional<T>::._anon_34::_value may be
               used uninitialized [-Wmaybe-uninitialized]" at every point an
               Optional with a trivial T (enums, integers...) is accessed due
               to not being able to see that the value is accessed only if it's
               actually set. The same issue was with std::optional apparently:
                https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80635
               One of the workarounds mentioned there is to add a second
               volatile member, some comments say it works only for some sizes
               of T so I'm making it the same size as _value. Is reproducible
               in Magnum and Magnum Plugins but not for example directly in
               OptionalTest so it's hard to see what all versions are affected.
               So just enabling this always. */
            /** @todo re-check with GCC 14, find a still-open upstream
                bugreport matching this problem */
            #if defined(CORRADE_TARGET_GCC) && !defined(CORRADE_TARGET_CLANG) && __OPTIMIZE__
            volatile char _gccStopSayingThisIsMaybeUninitialized[sizeof(T)];
            #endif
        };
        bool _set;
};

/** @relates Optional
@brief Equality comparison of a null optional and an optional

See @ref Optional::operator==(NullOptT) const for more information.
*/
template<class T> bool operator==(NullOptT, const Optional<T>& b) { return b == NullOpt; }

/** @relates Optional
@brief Non-euality comparison of a null optional and an optional

See @ref Optional::operator!=(NullOptT) const for more information.
*/
template<class T> bool operator!=(NullOptT, const Optional<T>& b) { return b != NullOpt; }

/** @relates Optional
@brief Equality comparison of a value and an optional

See @ref Optional::operator==(const T&) const for more information.
*/
template<class T> bool operator==(const T& a, const Optional<T>& b) { return b == a; }

/** @relates Optional
@brief Non-equality comparison of a value and an optional

See @ref Optional::operator!=(const T&) const for more information.
*/
template<class T> bool operator!=(const T& a, const Optional<T>& b) { return b != a; }

#ifndef DOXYGEN_GENERATING_OUTPUT
template<class T> class Optional<T&> {
    public:
        typedef T& Type;

        constexpr /*implicit*/ Optional(NullOptT = NullOpt) noexcept: _reference{} {}

        constexpr /*implicit*/ Optional(T& value) noexcept: _reference{&value} {}

        Optional(T&&) = delete;

        constexpr /*implicit*/ Optional(Corrade::InPlaceInitT, T& value) noexcept: _reference{&value} {}

        Optional(Corrade::InPlaceInitT, T&&) = delete;

        template<class U, class = typename std::enable_if<std::is_convertible<U*, T*>::value>::type> constexpr /*implicit*/ Optional(const Optional<U&>& other) noexcept: _reference{other._reference} {}

        template<class U, class = typename std::enable_if<!std::is_reference<U>::value && std::is_convertible<U*, T*>::value>::type> constexpr /*implicit*/ Optional(Optional<U>& other) noexcept: _reference{other._set ? &static_cast<U&>(other._value) : nullptr} {}

        template<class U, class = typename std::enable_if<!std::is_reference<U>::value && std::is_convertible<const U*, T*>::value>::type> constexpr /*implicit*/ Optional(const Optional<U>& other) noexcept: _reference{other._set ? &static_cast<const U&>(other._value) : nullptr} {}

        /* The value of an expiring Optional<U> would dangle */
        template<class U, class = typename std::enable_if<!std::is_reference<U>::value>::type> Optional(const Optional<U>&&) = delete;

        CORRADE_CONSTEXPR14 Optional<T&>& operator=(NullOptT) noexcept {
            _reference = nullptr;
            return *this;
        }

        constexpr explicit operator bool() const { return _reference != nullptr; }

        constexpr bool operator==(const Optional<T&>& other) const {
            return (!_reference && !other._reference) || (_reference && other._reference && *_reference == *other._reference);
        }

        constexpr bool operator!=(const Optional<T&>& other) const { return !operator==(other); }

        constexpr bool operator==(NullOptT) const { return !_reference; }

        constexpr bool operator!=(NullOptT) const { return _reference != nullptr; }

        /* For a volatile T, a const T& parameter couldn't bind a temporary */
        constexpr bool operator==(const typename std::remove_cv<T>::type& other) const {
            return _reference ? *_reference == other : false;
        }

        constexpr bool operator!=(const typename std::remove_cv<T>::type& other) const { return !operator==(other); }

        constexpr T* operator->() const {
            return CORRADE_CONSTEXPR_DEBUG_ASSERT(_reference, "Containers::Optional: the optional is empty"), _reference;
        }

        constexpr T& operator*() const {
            return CORRADE_CONSTEXPR_DEBUG_ASSERT(_reference, "Containers::Optional: the optional is empty"), *_reference;
        }

        CORRADE_CONSTEXPR14 T& emplace(T& value) noexcept {
            _reference = &value;
            return value;
        }

        T& emplace(T&&) = delete;

    private:
        template<class> friend class Optional;

        T* _reference;
};

template<class T> class Optional<T&&> {
    static_assert(!std::is_rvalue_reference<T&&>::value, "Containers::Optional: rvalue references are not supported");
};

template<class T> constexpr bool operator==(NullOptT, const Optional<T&>& b) { return b == NullOpt; }

template<class T> constexpr bool operator!=(NullOptT, const Optional<T&>& b) { return b != NullOpt; }

/* The overloads above can't deduce T from a value and an Optional<T&> */
template<class T> constexpr bool operator==(const typename std::remove_cv<T>::type& a, const Optional<T&>& b) { return b == a; }

template<class T> constexpr bool operator!=(const typename std::remove_cv<T>::type& a, const Optional<T&>& b) { return b != a; }
#endif

namespace Implementation {
    /* The default implementation has a Type member, but the user-provided
       ones don't. This is used to disambiguate whether optional(T&&) should
       return Optional<T> or U where U is result of the conversion from T. */
    template<class T> struct DeducedOptionalConverter { typedef T Type; };
}

/** @relatesalso Optional
@brief Make an optional

Convenience alternative to @ref Optional::Optional(const T&) or @ref Optional::Optional(T&&).
The following two lines are equivalent:

@snippet Containers.cpp optional

@attention Note that for types that are constructible from their own non-
    @cpp const @ce reference the call gets ambiguous between this function and
    @ref optional(Args&&... args). Such case is impossible to detect at compile
    time and you're advised to use the @ref Optional constructor explicitly to
    avoid surprising behavior.

@see @ref optional(Args&&... args), @ref pointer(T*)
*/
template<class T> inline
#ifdef DOXYGEN_GENERATING_OUTPUT
Optional<typename std::decay<T>::type>
#else
Optional<typename Implementation::DeducedOptionalConverter<typename std::decay<T>::type>::Type>
#endif
optional(T&& value) {
    return Optional<typename std::decay<T>::type>{Utility::forward<T>(value)};
}

/** @relatesalso Optional
@brief Make an optional

Convenience alternative to @ref Optional::Optional(InPlaceInitT, Args&&... args).
The following two lines are equivalent:

@snippet Containers.cpp optional-inplace

@attention Note that for types that are constructible from their own non-
    @cpp const @ce reference the call gets ambiguous between this function and
    @ref optional(T&&). Such case is impossible to detect at compile time and
    you're advised to use the @ref Optional constructor explicitly to avoid
    surprising behavior.

@see @ref optional(T&&), @ref pointer(Args&&... args)
*/
template<class T, class ...Args> inline Optional<T> optional(Args&&... args) {
    return Optional<T>{Corrade::InPlaceInit, Utility::forward<Args>(args)...};
}

/** @relatesalso Optional
@brief Make an optional from external representation

@see @ref Containers-Optional-stl
*/
template<class T> inline auto optional(T&& other) -> decltype(Implementation::DeducedOptionalConverter<typename std::decay<T>::type>::from(Utility::forward<T>(other))) {
    return Implementation::DeducedOptionalConverter<typename std::decay<T>::type>::from(Utility::forward<T>(other));
}

#ifndef CORRADE_SINGLES_NO_DEBUG
/** @debugoperator{NullOptT} */
inline Utility::Debug& operator<<(Utility::Debug& debug, NullOptT) {
    return debug << "Containers::NullOpt";
}

/** @debugoperator{Optional} */
template<class T> Utility::Debug& operator<<(Utility::Debug& debug, const Optional<T>& value) {
    if(!value)
        return debug << NullOpt;
    else return debug << *value;
}
#endif

template<class T> Optional<T>& Optional<T>::operator=(NullOptT) noexcept {
    if(_set) {
        _value.~T();
        poisonValue();
    }
    _set = false;
    return *this;
}

template<class T> template<class ...Args> T& Optional<T>::emplace(Args&&... args) {
    /* Done like this instead of Utility::swap() so it works for non-movable /
       non-copyable types as well. */
    if(_set)
        _value.~T();
    else
        unpoisonValue();
    Implementation::construct<typename std::remove_const<T>::type>(_value, Utility::forward<Args>(args)...);
    /* For exception safety flip the _set bit only after the constructor
       finished -- if the constructor would throw, this means we don't call
       a destructor on a partially-constructed instance, which would most
       definitely blow up. */
    /** @todo operator=() probably needs the same treatment? */
    _set = true;
    return _value;
}

}}

#undef _CORRADE_CONTAINERS_OPTIONAL_SANITIZER_ENABLED

#endif
