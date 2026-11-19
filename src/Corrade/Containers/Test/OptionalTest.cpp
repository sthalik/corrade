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

#include <vector> /* for verification that moving works with STL containers */

#include "Corrade/Containers/Optional.h"
#include "Corrade/Containers/Pair.h"
#include "Corrade/Containers/String.h"
#include "Corrade/TestSuite/Tester.h"

namespace {

struct MaybeInt {
    MaybeInt(int a): a{a} {}

    int a;
};

struct MaybePtr {
    MaybePtr(int* a): a{a} {}
    MaybePtr(const MaybePtr&) = delete;
    MaybePtr(MaybePtr&& other): a{other.a} {
        other.a = nullptr;
    }
    ~MaybePtr() { delete a; }
    MaybePtr& operator=(const MaybePtr&) = delete;
    /* Some compilers get upset when move assignment is not implemented even
       though it's not actually used */
    CORRADE_UNUSED MaybePtr& operator=(MaybePtr&& other) {
        Corrade::Utility::swap(a, other.a);
        return *this;
    }

    int* a;
};

}

namespace Corrade { namespace Containers {

namespace Implementation {

template<> struct OptionalConverter<int, MaybeInt> {
    static Optional<int> from(const MaybeInt& other) {
        return other.a;
    }

    static MaybeInt to(const Optional<int>& other) {
        return *other;
    }
};

template<> struct DeducedOptionalConverter<MaybeInt>: OptionalConverter<int, MaybeInt> {};

template<> struct OptionalConverter<int*, MaybePtr> {
    static Optional<int*> from(MaybePtr&& other) {
        Optional<int*> ret{other.a};
        other.a = nullptr;
        return ret;
    }

    static MaybePtr to(Optional<int*>&& other) {
        MaybePtr ret{*other};
        other = NullOpt;
        return ret;
    }
};

template<> struct DeducedOptionalConverter<MaybePtr>: OptionalConverter<int*, MaybePtr> {};

}

namespace Test { namespace {

struct OptionalTest: TestSuite::Tester {
    explicit OptionalTest();

    void nullOptNoDefaultConstructor();
    void nullOptInlineDefinition();

    void constructDefault();
    void constructNullOpt();
    void constructCopy();
    void constructCopyMake();
    void constructMove();
    void constructMoveMake();
    void constructInPlace();
    void constructInPlaceMake();
    void constructInPlaceMakeAmbiguous();
    void convertCopy();
    void convertMove();

    void constructCopyFromNull();
    void constructCopyFromSet();

    void constructMoveFromNull();
    void constructMoveFromSet();

    void boolConversion();

    void compareToOptional();
    void compareToNull();
    void compareToValue();

    void copyNullToNull();
    void copyNullToSet();
    void copySetToNull();
    void copySetToSet();

    void moveNullToNull();
    void moveNullToSet();
    void moveSetToNull();
    void moveSetToSet();

    void moveNullOptToNull();
    void moveNullOptToSet();

    void assignEmptyInitializerList();

    void emplaceNull();
    void emplaceSet();
    void emplaceException();

    void resetCounters();

    void access();
    void accessRvalue();
    void accessRvalueLifetimeExtension();
    void accessInvalid();

    void debug();

    void constructorExplicitInCopyInitialization();
    void copyConstructPlainStruct();
    void moveConstructPlainStruct();
    void vectorOfMovableOptional();

    void constructConst();
    void emplaceConst();

    void constructReference();
    void constructReferenceConstexpr();
    void constructReferenceFromReference();
    void constructReferenceFromValue();
    void assignReference();
    void emplaceReference();
    void compareReference();
    void accessReference();
    void accessReferenceVolatile();
    void accessReferenceInvalid();
    void debugReference();
};

OptionalTest::OptionalTest() {
    addTests({&OptionalTest::nullOptNoDefaultConstructor,
              &OptionalTest::nullOptInlineDefinition});

    addTests({&OptionalTest::constructDefault,
              &OptionalTest::constructNullOpt,
              &OptionalTest::constructCopy,
              &OptionalTest::constructCopyMake,
              &OptionalTest::constructMove,
              &OptionalTest::constructMoveMake,
              &OptionalTest::constructInPlace,
              &OptionalTest::constructInPlaceMake,
              &OptionalTest::constructInPlaceMakeAmbiguous}, &OptionalTest::resetCounters, &OptionalTest::resetCounters);

    addTests({&OptionalTest::convertCopy,
              &OptionalTest::convertMove});

    addTests({&OptionalTest::constructCopyFromNull,
              &OptionalTest::constructCopyFromSet,

              &OptionalTest::constructMoveFromNull,
              &OptionalTest::constructMoveFromSet,

              &OptionalTest::boolConversion,

              &OptionalTest::compareToOptional,
              &OptionalTest::compareToNull,
              &OptionalTest::compareToValue,

              &OptionalTest::copyNullToNull,
              &OptionalTest::copyNullToSet,
              &OptionalTest::copySetToNull,
              &OptionalTest::copySetToSet,

              &OptionalTest::moveNullToNull,
              &OptionalTest::moveNullToSet,
              &OptionalTest::moveSetToNull,
              &OptionalTest::moveSetToSet,

              &OptionalTest::moveNullOptToNull,
              &OptionalTest::moveNullOptToSet,

              &OptionalTest::assignEmptyInitializerList,

              &OptionalTest::emplaceNull,
              &OptionalTest::emplaceSet}, &OptionalTest::resetCounters, &OptionalTest::resetCounters);

    addTests({&OptionalTest::emplaceException,

              &OptionalTest::access,
              &OptionalTest::accessRvalue,
              &OptionalTest::accessRvalueLifetimeExtension,
              &OptionalTest::accessInvalid,

              &OptionalTest::debug,

              &OptionalTest::constructorExplicitInCopyInitialization,
              &OptionalTest::copyConstructPlainStruct,
              &OptionalTest::moveConstructPlainStruct,
              &OptionalTest::vectorOfMovableOptional});

    addTests({&OptionalTest::constructConst,
              &OptionalTest::emplaceConst}, &OptionalTest::resetCounters, &OptionalTest::resetCounters);

    addTests({&OptionalTest::constructReference,
              &OptionalTest::constructReferenceConstexpr,
              &OptionalTest::constructReferenceFromReference,
              &OptionalTest::constructReferenceFromValue,
              &OptionalTest::assignReference,
              &OptionalTest::emplaceReference,
              &OptionalTest::compareReference,
              &OptionalTest::accessReference,
              &OptionalTest::accessReferenceVolatile,
              &OptionalTest::accessReferenceInvalid,
              &OptionalTest::debugReference});
}

void OptionalTest::nullOptNoDefaultConstructor() {
    CORRADE_VERIFY(!std::is_default_constructible<NullOptT>::value);
}

void OptionalTest::nullOptInlineDefinition() {
    CORRADE_VERIFY(std::is_same<decltype(NullOpt), const NullOptT>::value);
}

struct Throwable {
    explicit Throwable(int) {}
    Throwable(const Throwable&) {}
    Throwable(Throwable&&) {}
    Throwable& operator=(const Throwable&) { return *this; }
    Throwable& operator=(Throwable&&) { return *this; }
};

struct Copyable {
    static int constructed;
    static int destructed;
    static int copied;
    static int moved;

    explicit Copyable(int a) noexcept: a{a} { ++constructed; }
    Copyable(const Copyable& other) noexcept: a{other.a} {
        ++constructed;
        ++copied;
    }
    Copyable(Copyable&& other) noexcept: a{other.a} {
        ++constructed;
        ++moved;
    }
    ~Copyable() { ++destructed; }
    Copyable& operator=(const Copyable& other) noexcept {
        a = other.a;
        ++copied;
        return *this;
    }
    Copyable& operator=(Copyable&& other) noexcept {
        a = other.a;
        ++moved;
        return *this;
    }

    int a;
};

int Copyable::constructed = 0;
int Copyable::destructed = 0;
int Copyable::copied = 0;
int Copyable::moved = 0;

struct Movable {
    static int constructed;
    static int destructed;
    static int moved;

    explicit Movable(int a) noexcept: a{a} { ++constructed; }
    /* To test perfect forwarding in in-place construction */
    explicit Movable(int a, int&&) noexcept: Movable{a} {}
    Movable(const Movable&) = delete;
    Movable(Movable&& other) noexcept: a(other.a) {
        ++constructed;
        ++moved;
    }
    ~Movable() { ++destructed; }
    Movable& operator=(const Movable&) = delete;
    Movable& operator=(Movable&& other) noexcept {
        a = other.a;
        ++moved;
        return *this;
    }

    int a;
};

int Movable::constructed = 0;
int Movable::destructed = 0;
int Movable::moved = 0;

struct Immovable {
    static int constructed;
    static int destructed;

    Immovable(const Immovable&) = delete;
    Immovable(Immovable&&) = delete;
    explicit Immovable(int a) noexcept: a{a} { ++constructed; }
    /* To test perfect forwarding in in-place construction */
    explicit Immovable(int a, int&&) noexcept: Immovable{a} {}
    ~Immovable() { ++destructed; }
    Immovable& operator=(const Immovable&) = delete;
    Immovable& operator=(Immovable&&) = delete;

    int a;
};

int Immovable::constructed = 0;
int Immovable::destructed = 0;

void swap(Movable& a, Movable& b) {
    /* Swap these without copying the parent class */
    Utility::swap(a.a, b.a);
}

void OptionalTest::constructDefault() {
    {
        Optional<Copyable> a;
        CORRADE_VERIFY(!a);
    }

    CORRADE_COMPARE(Copyable::constructed, 0);
    CORRADE_COMPARE(Copyable::destructed, 0);
    CORRADE_COMPARE(Copyable::copied, 0);
    CORRADE_COMPARE(Copyable::moved, 0);

    CORRADE_VERIFY(std::is_nothrow_constructible<Optional<Copyable>>::value);
    CORRADE_VERIFY(std::is_nothrow_constructible<Optional<Throwable>>::value);
}

void OptionalTest::constructNullOpt() {
    {
        Optional<Copyable> a{NullOpt};
        Optional<Copyable> b = NullOpt;
        CORRADE_VERIFY(!a);
        CORRADE_VERIFY(!b);
    }

    CORRADE_COMPARE(Copyable::constructed, 0);
    CORRADE_COMPARE(Copyable::destructed, 0);
    CORRADE_COMPARE(Copyable::copied, 0);
    CORRADE_COMPARE(Copyable::moved, 0);

    CORRADE_VERIFY(std::is_nothrow_constructible<Optional<Copyable>, NullOptT>::value);
    CORRADE_VERIFY(std::is_nothrow_constructible<Optional<Throwable>, NullOptT>::value);
}

void OptionalTest::constructCopy() {
    /* copy construction tested below in constructCopyFrom*()/ copy*To*() */

    {
        Copyable v{32};
        Optional<Copyable> a{v};
        CORRADE_VERIFY(a);
        CORRADE_COMPARE(a->a, 32);
    }

    CORRADE_COMPARE(Copyable::constructed, 2);
    CORRADE_COMPARE(Copyable::destructed, 2);
    CORRADE_COMPARE(Copyable::copied, 1);
    CORRADE_COMPARE(Copyable::moved, 0);

    CORRADE_VERIFY(std::is_nothrow_copy_constructible<Copyable>::value);
    CORRADE_VERIFY(std::is_nothrow_copy_constructible<Optional<Copyable>>::value);
    CORRADE_VERIFY(std::is_nothrow_constructible<Optional<Copyable>, const Copyable&>::value);
    CORRADE_VERIFY(std::is_nothrow_copy_assignable<Copyable>::value);
    CORRADE_VERIFY(std::is_nothrow_copy_assignable<Optional<Copyable>>::value);
    CORRADE_VERIFY(std::is_nothrow_assignable<Optional<Copyable>, const Copyable&>::value);

    CORRADE_VERIFY(std::is_copy_constructible<Throwable>::value);
    CORRADE_VERIFY(!std::is_nothrow_copy_constructible<Throwable>::value);
    CORRADE_VERIFY(!std::is_nothrow_copy_constructible<Optional<Throwable>>::value);
    CORRADE_VERIFY(!std::is_nothrow_constructible<Optional<Throwable>, const Throwable&>::value);
    CORRADE_VERIFY(std::is_copy_assignable<Throwable>::value);
    CORRADE_VERIFY(!std::is_nothrow_copy_assignable<Throwable>::value);
    CORRADE_VERIFY(!std::is_nothrow_copy_assignable<Optional<Throwable>>::value);
    CORRADE_VERIFY(!std::is_nothrow_assignable<Optional<Throwable>, const Throwable&>::value);
}

void OptionalTest::constructCopyMake() {
    {
        Copyable v{32};
        auto a = optional(v);
        CORRADE_VERIFY(a);
        CORRADE_VERIFY(std::is_same<decltype(a), Optional<Copyable>>::value);
        CORRADE_COMPARE(a->a, 32);
    }

    CORRADE_COMPARE(Copyable::constructed, 2);
    CORRADE_COMPARE(Copyable::destructed, 2);
    /* Argument is forwarded (passed as T&), so there's just one copy: on return */
    CORRADE_COMPARE(Copyable::copied, 1);
}

void OptionalTest::constructMove() {
    /* move construction tested below in constructMoveFrom*()/ move*To*() */

    {
        Optional<Movable> a{Movable{32}};
        CORRADE_VERIFY(a);
        CORRADE_COMPARE(a->a, 32);
    }

    CORRADE_COMPARE(Movable::constructed, 2);
    CORRADE_COMPARE(Movable::destructed, 2);
    CORRADE_COMPARE(Movable::moved, 1);

    CORRADE_VERIFY(!std::is_copy_constructible<Movable>::value);
    CORRADE_VERIFY(!std::is_copy_assignable<Movable>::value);
    CORRADE_VERIFY(!std::is_copy_constructible<Optional<Movable>>::value);
    CORRADE_VERIFY(!std::is_copy_assignable<Optional<Movable>>::value);

    CORRADE_VERIFY(std::is_nothrow_move_constructible<Movable>::value);
    CORRADE_VERIFY(std::is_nothrow_move_constructible<Optional<Movable>>::value);
    CORRADE_VERIFY(std::is_nothrow_constructible<Optional<Movable>, Movable&&>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<Movable>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<Optional<Movable>>::value);
    CORRADE_VERIFY(std::is_nothrow_assignable<Optional<Movable>, Movable&&>::value);

    CORRADE_VERIFY(std::is_move_constructible<Throwable>::value);
    CORRADE_VERIFY(!std::is_nothrow_move_constructible<Throwable>::value);
    CORRADE_VERIFY(!std::is_nothrow_move_constructible<Optional<Throwable>>::value);
    CORRADE_VERIFY(!std::is_nothrow_constructible<Optional<Throwable>, Throwable&&>::value);
    CORRADE_VERIFY(std::is_move_assignable<Throwable>::value);
    CORRADE_VERIFY(!std::is_nothrow_move_assignable<Throwable>::value);
    CORRADE_VERIFY(!std::is_nothrow_move_assignable<Optional<Throwable>>::value);
    CORRADE_VERIFY(!std::is_nothrow_assignable<Optional<Throwable>, Throwable&&>::value);
}

void OptionalTest::constructMoveMake() {
    {
        auto a = optional(Movable{32});
        CORRADE_VERIFY(a);
        CORRADE_VERIFY(std::is_same<decltype(a), Optional<Movable>>::value);
        CORRADE_COMPARE(a->a, 32);
    }

    CORRADE_COMPARE(Movable::constructed, 2);
    CORRADE_COMPARE(Movable::destructed, 2);
    /* Argument is forwarded (passed as T&&), so there's just one move: on return */
    CORRADE_COMPARE(Movable::moved, 1);
}

void OptionalTest::constructInPlace() {
    {
        /* Using int{} to test perfect forwarding */
        Optional<Immovable> a{Corrade::InPlaceInit, 32, int{}};
        CORRADE_VERIFY(a);
        CORRADE_COMPARE(a->a, 32);
    }

    CORRADE_COMPARE(Immovable::constructed, 1);
    CORRADE_COMPARE(Immovable::destructed, 1);

    CORRADE_VERIFY(!std::is_copy_constructible<Immovable>::value);
    CORRADE_VERIFY(!std::is_move_constructible<Immovable>::value);
    CORRADE_VERIFY(!std::is_copy_assignable<Immovable>::value);
    CORRADE_VERIFY(!std::is_move_assignable<Immovable>::value);
    CORRADE_VERIFY(!std::is_copy_constructible<Optional<Immovable>>::value);
    CORRADE_VERIFY(!std::is_move_constructible<Optional<Immovable>>::value);
    CORRADE_VERIFY(!std::is_copy_assignable<Optional<Immovable>>::value);
    CORRADE_VERIFY(!std::is_move_assignable<Optional<Immovable>>::value);

    CORRADE_VERIFY(std::is_nothrow_constructible<Optional<Immovable>, Corrade::InPlaceInitT, int, int&&>::value);
    CORRADE_VERIFY(std::is_constructible<Optional<Throwable>, Corrade::InPlaceInitT, int>::value);
    CORRADE_VERIFY(!std::is_nothrow_constructible<Optional<Throwable>, Corrade::InPlaceInitT, int>::value);
}

void OptionalTest::constructInPlaceMake() {
    {
        /* Using int{} to test perfect forwarding */
        auto a = optional<Movable>(15, int{});
        CORRADE_VERIFY(a);
        CORRADE_COMPARE(a->a, 15);
    }

    /* Interesting. So there is a full RVO? */
    CORRADE_COMPARE(Movable::constructed, 1);
    CORRADE_COMPARE(Movable::destructed, 1);
    CORRADE_COMPARE(Movable::moved, 0);
}

void OptionalTest::constructInPlaceMakeAmbiguous() {
    struct Ambiguous {
        Ambiguous() = default;
        Ambiguous(Ambiguous& parent, int = {}): parent{&parent} {}
        /* Still broken even on MSVC 2022. Maybe 2025 will be the year when
           MSVC can finally do plain C++11? */
        #if !defined(CORRADE_TARGET_MSVC) || defined(CORRADE_TARGET_CLANG_CL) || _MSC_VER >= 1940
        /* https://developercommunity.visualstudio.com/content/problem/358751/c2580-for-different-versions-of-constructors.html,
           affects 2015 as well (https://stackoverflow.com/a/36658141) */
        Ambiguous(const Ambiguous&) = default;
        #else
        Ambiguous(const Ambiguous& other): parent{other.parent} {}
        #endif
        Ambiguous& operator=(const Ambiguous&) = default;
        Ambiguous* parent{};
    };

    /* Similar to what's in the Pointer test, though there it is a
       static_assert. Here we can't disambiguate. */
    Ambiguous parent;
    auto a = optional(parent);
    auto b = optional(Ambiguous{});
    auto c = optional<Ambiguous>(parent);
    auto d = optional<Ambiguous>(Ambiguous{});
    auto e = optional<Ambiguous>();
    auto f = optional<Ambiguous>(parent, 32);
    auto g = Optional<Ambiguous>{Corrade::InPlaceInit, parent};
    auto h = Optional<Ambiguous>{parent};
    CORRADE_COMPARE(a->parent, nullptr); /* wrong, but we can't disambiguate */
    CORRADE_COMPARE(b->parent, nullptr);
    CORRADE_COMPARE(c->parent, &parent);
    CORRADE_COMPARE(d->parent, nullptr);
    CORRADE_COMPARE(e->parent, nullptr);
    CORRADE_COMPARE(f->parent, &parent);
    CORRADE_COMPARE(g->parent, &parent);
    CORRADE_COMPARE(h->parent, nullptr);
}

void OptionalTest::convertCopy() {
    MaybeInt a(5);
    CORRADE_COMPARE(a.a, 5);

    Optional<int> b(a);
    CORRADE_VERIFY(b);
    CORRADE_COMPARE(*b, 5);

    MaybeInt c(b);
    CORRADE_COMPARE(c.a, 5);

    auto d = optional(MaybeInt{35});
    CORRADE_VERIFY(std::is_same<decltype(d), Optional<int>>::value);
    CORRADE_VERIFY(d);
    CORRADE_COMPARE(*d, 35);

    /* Conversion from a different type is not allowed */
    CORRADE_VERIFY(std::is_constructible<Optional<int>, MaybeInt>::value);
    CORRADE_VERIFY(!std::is_constructible<Optional<float>, MaybeInt>::value);
    CORRADE_VERIFY(std::is_constructible<MaybeInt, Optional<int>>::value);
    CORRADE_VERIFY(!std::is_constructible<MaybeInt, Optional<float>>::value);

    /* Implicit conversion is not allowed */
    CORRADE_VERIFY(!std::is_convertible<const MaybeInt&, Optional<int>>::value);
    CORRADE_VERIFY(!std::is_convertible<const Optional<int>&, MaybeInt>::value);
}

void OptionalTest::convertMove() {
    MaybePtr a(new int{35});
    CORRADE_COMPARE(*a.a, 35);

    Optional<int*> b(Utility::move(a));
    CORRADE_VERIFY(b);
    CORRADE_VERIFY(*b);
    CORRADE_COMPARE(**b, 35);
    CORRADE_VERIFY(!a.a);

    MaybePtr c(Utility::move(b));
    CORRADE_VERIFY(c.a);
    CORRADE_COMPARE(*c.a, 35);
    CORRADE_VERIFY(!b);

    int dv = 17; /* to avoid a leak */
    auto d = optional(MaybePtr{&dv});
    CORRADE_VERIFY(std::is_same<decltype(d), Optional<int*>>::value);
    CORRADE_VERIFY(d);
    CORRADE_VERIFY(*d);
    CORRADE_COMPARE(**d, 17);

    /* Conversion from a different type is not allowed */
    CORRADE_VERIFY(std::is_constructible<Optional<int*>, MaybePtr&&>::value);
    CORRADE_VERIFY(!std::is_constructible<Optional<float*>, MaybePtr&&>::value);
    CORRADE_VERIFY(std::is_constructible<MaybePtr, Optional<int*>&&>::value);
    CORRADE_VERIFY(!std::is_constructible<MaybePtr, Optional<float*>&&>::value);

    /* Copy construction is not allowed */
    CORRADE_VERIFY(!std::is_constructible<Optional<int*>, MaybePtr&>::value);
    CORRADE_VERIFY(!std::is_constructible<Optional<int*>, const MaybePtr&>::value);
    CORRADE_VERIFY(!std::is_constructible<MaybePtr, Optional<int*>&>::value);
    CORRADE_VERIFY(!std::is_constructible<MaybePtr, const Optional<int*>&>::value);

    /* Implicit conversion is not allowed */
    CORRADE_VERIFY(!std::is_convertible<MaybePtr&&, Optional<int*>>::value);
    CORRADE_VERIFY(!std::is_convertible<Optional<int*>&&, MaybePtr>::value);
}

void OptionalTest::constructCopyFromNull() {
    {
        Optional<Copyable> a;
        Optional<Copyable> b{a};

        CORRADE_VERIFY(!a);
        CORRADE_VERIFY(!b);
    }

    CORRADE_COMPARE(Copyable::constructed, 0);
    CORRADE_COMPARE(Copyable::destructed, 0);
    CORRADE_COMPARE(Copyable::copied, 0);
    CORRADE_COMPARE(Copyable::moved, 0);
}

void OptionalTest::constructCopyFromSet() {
    {
        Optional<Copyable> a{Corrade::InPlaceInit, 32};
        Optional<Copyable> b{a};

        CORRADE_VERIFY(a);
        CORRADE_VERIFY(b);
    }

    CORRADE_COMPARE(Copyable::constructed, 2);
    CORRADE_COMPARE(Copyable::destructed, 2);
    CORRADE_COMPARE(Copyable::copied, 1);
    CORRADE_COMPARE(Copyable::moved, 0);
}

void OptionalTest::constructMoveFromNull() {
    {
        Optional<Copyable> a;
        Optional<Copyable> b{Utility::move(a)};

        CORRADE_VERIFY(!a);
        CORRADE_VERIFY(!b);
    }

    CORRADE_COMPARE(Copyable::constructed, 0);
    CORRADE_COMPARE(Copyable::destructed, 0);
    CORRADE_COMPARE(Copyable::copied, 0);
    CORRADE_COMPARE(Copyable::moved, 0);
}

void OptionalTest::constructMoveFromSet() {
    {
        Optional<Copyable> a{Corrade::InPlaceInit, 32};
        Optional<Copyable> b{Utility::move(a)};

        /* A is still set, because the type destructor needs to be called */
        CORRADE_VERIFY(a);
        CORRADE_VERIFY(b);
    }

    CORRADE_COMPARE(Copyable::constructed, 2);
    CORRADE_COMPARE(Copyable::destructed, 2);
    CORRADE_COMPARE(Copyable::copied, 0);
    CORRADE_COMPARE(Copyable::moved, 1);
}

void OptionalTest::boolConversion() {
    Optional<int> a;
    Optional<int> b{5};

    CORRADE_VERIFY(!a);
    CORRADE_VERIFY(b);
    CORRADE_VERIFY(!!b);

    /* Implicit conversion is not allowed */
    CORRADE_VERIFY(!std::is_convertible<Optional<int>, int>::value);
    CORRADE_VERIFY(!std::is_convertible<Optional<int>, bool>::value);
}

void OptionalTest::compareToOptional() {
    Optional<int> a;
    Optional<int> b{5};
    Optional<int> c{6};

    CORRADE_VERIFY(a == a);
    CORRADE_VERIFY(b == b);
    CORRADE_VERIFY(a != b);
    CORRADE_VERIFY(b != a);
    CORRADE_VERIFY(b != c);
    CORRADE_VERIFY(c != b);
}

void OptionalTest::compareToNull() {
    Optional<int> a;
    Optional<int> b{5};

    CORRADE_VERIFY(a == NullOpt);
    CORRADE_VERIFY(b != NullOpt);
    CORRADE_VERIFY(NullOpt == a);
    CORRADE_VERIFY(NullOpt != b);
}

void OptionalTest::compareToValue() {
    Optional<int> a;
    Optional<int> b{5};
    Optional<int> c{6};

    CORRADE_VERIFY(a != 6);
    CORRADE_VERIFY(b != 6);
    CORRADE_VERIFY(c == 6);
    CORRADE_VERIFY(6 != a);
    CORRADE_VERIFY(6 != b);
    CORRADE_VERIFY(6 == c);
}

void OptionalTest::copyNullToNull() {
    {
        Optional<Copyable> a;
        Optional<Copyable> b;
        b = a;

        CORRADE_VERIFY(!a);
        CORRADE_VERIFY(!b);
    }

    CORRADE_COMPARE(Copyable::constructed, 0);
    CORRADE_COMPARE(Copyable::destructed, 0);
    CORRADE_COMPARE(Copyable::copied, 0);
    CORRADE_COMPARE(Copyable::moved, 0);
}

void OptionalTest::copyNullToSet() {
    {
        Optional<Copyable> a;
        Optional<Copyable> b{Corrade::InPlaceInit, 32};
        b = a;

        CORRADE_VERIFY(!a);
        CORRADE_VERIFY(!b);
    }

    CORRADE_COMPARE(Copyable::constructed, 1);
    CORRADE_COMPARE(Copyable::destructed, 1);
    CORRADE_COMPARE(Copyable::copied, 0);
    CORRADE_COMPARE(Copyable::moved, 0);
}

void OptionalTest::copySetToNull() {
    {
        Optional<Copyable> a{Corrade::InPlaceInit, 32};
        Optional<Copyable> b;
        b = a;

        CORRADE_VERIFY(a);
        CORRADE_VERIFY(b);
        CORRADE_COMPARE(b->a, 32);
    }

    CORRADE_COMPARE(Copyable::constructed, 2);
    CORRADE_COMPARE(Copyable::destructed, 2);
    CORRADE_COMPARE(Copyable::copied, 1);
    CORRADE_COMPARE(Copyable::moved, 0);
}

void OptionalTest::copySetToSet() {
    {
        Optional<Copyable> a{Corrade::InPlaceInit, 32};
        Optional<Copyable> b{Corrade::InPlaceInit, 78};
        b = a;

        CORRADE_VERIFY(a);
        CORRADE_VERIFY(b);
        CORRADE_COMPARE(b->a, 32);
    }

    CORRADE_COMPARE(Copyable::constructed, 3);
    CORRADE_COMPARE(Copyable::destructed, 3);
    CORRADE_COMPARE(Copyable::copied, 1);
    CORRADE_COMPARE(Copyable::moved, 0);
}

void OptionalTest::moveNullToNull() {
    {
        Optional<Movable> a;
        Optional<Movable> b;
        b = Utility::move(a);

        CORRADE_VERIFY(!a);
        CORRADE_VERIFY(!b);
    }

    CORRADE_COMPARE(Movable::constructed, 0);
    CORRADE_COMPARE(Movable::destructed, 0);
    CORRADE_COMPARE(Movable::moved, 0);
}

void OptionalTest::moveNullToSet() {
    {
        Optional<Movable> a;
        Optional<Movable> b{Corrade::InPlaceInit, 32};
        b = Utility::move(a);

        CORRADE_VERIFY(!a);
        CORRADE_VERIFY(!b);
    }

    CORRADE_COMPARE(Movable::constructed, 1);
    CORRADE_COMPARE(Movable::destructed, 1);
    CORRADE_COMPARE(Movable::moved, 0);
}

void OptionalTest::moveSetToNull() {
    {
        Optional<Movable> a{Corrade::InPlaceInit, 32};
        Optional<Movable> b;
        b = Utility::move(a);

        CORRADE_VERIFY(a);
        CORRADE_VERIFY(b);
        CORRADE_COMPARE(b->a, 32);
    }

    CORRADE_COMPARE(Movable::constructed, 2);
    CORRADE_COMPARE(Movable::destructed, 2);
    CORRADE_COMPARE(Movable::moved, 1);
}

void OptionalTest::moveSetToSet() {
    {
        Optional<Copyable> a{Corrade::InPlaceInit, 32};
        Optional<Copyable> b{Corrade::InPlaceInit, 78};
        b = Utility::move(a);

        CORRADE_VERIFY(a);
        CORRADE_VERIFY(b);
        CORRADE_COMPARE(b->a, 32);
    }

    /* One extra construction due to the temporary inside Utility::swap() */
    CORRADE_COMPARE(Copyable::constructed, 3);
    CORRADE_COMPARE(Copyable::destructed, 3);
    CORRADE_COMPARE(Copyable::copied, 0);
    /* A to temp, B to A, temp to B */
    CORRADE_COMPARE(Copyable::moved, 3);

    {
        Optional<Movable> a{Corrade::InPlaceInit, 32};
        Optional<Movable> b{Corrade::InPlaceInit, 78};
        b = Utility::move(a);

        CORRADE_VERIFY(a);
        CORRADE_VERIFY(b);
        CORRADE_COMPARE(b->a, 32);
    }

    /* The local swap() specialization should take effect, no temporary Movable
       object created and thus just two objects constructed and nothing moved */
    CORRADE_COMPARE(Movable::constructed, 2);
    CORRADE_COMPARE(Movable::destructed, 2);
    CORRADE_COMPARE(Movable::moved, 0);
}

void OptionalTest::moveNullOptToNull() {
    {
        Optional<Immovable> a;
        a = NullOpt;

        CORRADE_VERIFY(!a);
    }

    CORRADE_COMPARE(Immovable::constructed, 0);
    CORRADE_COMPARE(Immovable::destructed, 0);
}

void OptionalTest::moveNullOptToSet() {
    {
        Optional<Immovable> a{Corrade::InPlaceInit, 32};
        a = NullOpt;

        CORRADE_VERIFY(!a);
    }

    CORRADE_COMPARE(Immovable::constructed, 1);
    CORRADE_COMPARE(Immovable::destructed, 1);
}

void OptionalTest::assignEmptyInitializerList() {
    /* To verify the {} isn't taken as a default-constructed value, leading to
       the optional becoming non-null */

    Optional<Copyable> noDefaultConstructor;
    Optional<Containers::Pair<int, int>> copyable;
    Optional<Containers::Pointer<int>> movable;
    Optional<int> trivial;

    noDefaultConstructor = {};
    CORRADE_VERIFY(!noDefaultConstructor);

    copyable = {};
    CORRADE_VERIFY(!copyable);

    movable = {};
    CORRADE_VERIFY(!movable);

    trivial = {};
    CORRADE_VERIFY(!trivial);
}

void OptionalTest::emplaceNull() {
    {
        Optional<Immovable> a;
        /* Using int{} to test perfect forwarding */
        a.emplace(32, int{});

        CORRADE_VERIFY(a);
        CORRADE_COMPARE(a->a, 32);
    }

    CORRADE_COMPARE(Immovable::constructed, 1);
    CORRADE_COMPARE(Immovable::destructed, 1);
}

void OptionalTest::emplaceSet() {
    {
        Optional<Immovable> a{Corrade::InPlaceInit, 32};
        /* Using int{} to test perfect forwarding */
        a.emplace(76, int{});

        CORRADE_VERIFY(a);
        CORRADE_COMPARE(a->a, 76);
    }

    CORRADE_COMPARE(Immovable::constructed, 2);
    CORRADE_COMPARE(Immovable::destructed, 2);
}

void OptionalTest::emplaceException() {
    /* We're unfortunately dealing with libraries that use exceptions (OpenEXR)
       and using an Optional to store state there. That means, emplace() has to
       be exception-aware and flip the "set" bit only after the constructor
       finished, not earlier, in order to avoid getting the destructor called
       on an instance that didn't finish construction. */

    struct Throws {
        Throws(bool& destructorCalled): destructorCalled(destructorCalled) {
            throw int{};
        }

        ~Throws() {
            destructorCalled = true;
        }

        bool& destructorCalled;
    };

    bool thrown = false;
    bool destructorCalled = false;

    /* It shouldn't mark the optional as set if it throws */
    {
        Optional<Throws> a;
        try {
            a.emplace(destructorCalled);
        } catch(int) {
            thrown = true;
        }

        CORRADE_VERIFY(!a);
        CORRADE_VERIFY(thrown);
        CORRADE_VERIFY(!destructorCalled);
    }

    CORRADE_VERIFY(!destructorCalled);
}

void OptionalTest::resetCounters() {
    Copyable::constructed = Copyable::destructed = Copyable::copied = Copyable::moved =
        Movable::constructed = Movable::destructed = Movable::moved =
            Immovable::constructed = Immovable::destructed = 0;
}

void OptionalTest::access() {
    Optional<Copyable> a{Copyable{32}};
    const Optional<Copyable> ca{Copyable{18}};

    CORRADE_VERIFY(a);
    CORRADE_VERIFY(ca);
    CORRADE_COMPARE(a->a, 32);
    CORRADE_COMPARE(ca->a, 18);
    CORRADE_COMPARE((*a).a, 32);
    CORRADE_COMPARE((*ca).a, 18);
}

void OptionalTest::accessRvalue() {
    Movable b = *Optional<Movable>{Corrade::InPlaceInit, 42};
    CORRADE_COMPARE(b.a, 42);
}

void OptionalTest::accessRvalueLifetimeExtension() {
    struct DiesLoudly {
        DiesLoudly() = default;
        DiesLoudly(const DiesLoudly&) = delete;
        DiesLoudly(DiesLoudly&& other): orphaned{true} {
            other.orphaned = false;
        }

        ~DiesLoudly() {
            if(orphaned)
                Debug{} << "dying!";
        }

        bool orphaned = true;
    };

    Containers::String out;
    Debug redirectOutput{&out};
    {
        /* Here a reference lifetime extension should kick in, causing the
           output of operator*() to be destroyed only at the end of scope, and
           not already at the ;. A more common case of this would be with
           temporary expressions in a range-for loop, such as with

            for(auto&& file: *Utility::Path::list(path))
                // if the lifetime was not extended, it would iterate an
                // already-destroyed Array
        */
        auto&& value = *Optional<DiesLoudly>{DiesLoudly{}};
        Debug{} << "shouldn't be dead yet";

        /* So the compiler doesn't complain about the variables being unused
           (even though it's a load-bearing reference) */
        CORRADE_VERIFY(&value);
    }
    CORRADE_COMPARE(out,
        "shouldn't be dead yet\n"
        "dying!\n");
}

void OptionalTest::accessInvalid() {
    CORRADE_SKIP_IF_NO_DEBUG_ASSERT();

    struct Innocent {
        void foo() const {}
    };

    Optional<Innocent> a;
    const Optional<Innocent> ca;

    CORRADE_VERIFY(!a);
    CORRADE_VERIFY(!ca);

    Containers::String out;
    {
        Error redirectError{&out};
        a->foo();
        ca->foo();
        (*a).foo();
        (*ca).foo();
    }
    CORRADE_COMPARE(out,
        "Containers::Optional: the optional is empty\n"
        "Containers::Optional: the optional is empty\n"
        "Containers::Optional: the optional is empty\n"
        "Containers::Optional: the optional is empty\n");
}

void OptionalTest::debug() {
    Containers::String out;
    Debug{&out} << optional(42) << Optional<int>{} << NullOpt;
    CORRADE_COMPARE(out, "42 Containers::NullOpt Containers::NullOpt\n");
}

void OptionalTest::constructorExplicitInCopyInitialization() {
    /* See constructHelpers.h for details about this compiler-specific issue */
    struct ExplicitDefault {
        explicit ExplicitDefault() = default;
    };

    struct ContainingExplicitDefaultWithImplicitConstructor {
        ExplicitDefault a;
    };

    /* This alone works */
    ContainingExplicitDefaultWithImplicitConstructor a;
    static_cast<void>(a);

    /* So this should too */
    Optional<ContainingExplicitDefaultWithImplicitConstructor> b{Corrade::InPlaceInit};
    Optional<ContainingExplicitDefaultWithImplicitConstructor> c;
    c.emplace();
    CORRADE_VERIFY(b);
    CORRADE_VERIFY(c);
}

void OptionalTest::copyConstructPlainStruct() {
    struct ExtremelyTrivial {
        int a;
        char b;
    };

    /* This needs special handling on GCC 4.8, where T{b} (copy-construction)
       attempts to convert ExtremelyTrivial to int to initialize the first
       argument and fails miserably. */
    const ExtremelyTrivial value{3, 'a'};
    Optional<ExtremelyTrivial> a{value};
    CORRADE_COMPARE(a->a, 3);

    /* This copy-constructs the wrapped value */
    Optional<ExtremelyTrivial> b = a;
    CORRADE_COMPARE(b->a, 3);

    /* This deletes and then copy-constructs the wrapped value */
    Optional<ExtremelyTrivial> c;
    c = b;
    CORRADE_COMPARE(c->a, 3);
}

void OptionalTest::moveConstructPlainStruct() {
    /* Can't make MoveOnlyStruct directly non-copyable because then we'd hit
       another GCC 4.8 bug where it can't be constructed using {} anymore. In
       other tests I simply add a (move-only) Array or Pointer member, but here
       I don't want to avoid a needless header dependency. */
    struct MoveOnlyPointer {
        MoveOnlyPointer(std::nullptr_t) {}
        MoveOnlyPointer(const MoveOnlyPointer&) = delete;
        MoveOnlyPointer(MoveOnlyPointer&&) = default;
        MoveOnlyPointer& operator=(const MoveOnlyPointer&) = delete;
        MoveOnlyPointer& operator=(MoveOnlyPointer&&) = default;

        std::nullptr_t a;
    };

    struct MoveOnlyStruct {
        int a;
        char c;
        MoveOnlyPointer b;
    };

    /* This needs special handling on GCC 4.8, where T{Utility::move(b)}
       attempts to convert MoveOnlyStruct to int to initialize the first
       argument and fails miserably. */
    Optional<MoveOnlyStruct> a{MoveOnlyStruct{3, 'a', nullptr}};
    CORRADE_COMPARE(a->a, 3);

    /* This copy-constructs the wrapped value */
    Optional<MoveOnlyStruct> b = Utility::move(a);
    CORRADE_COMPARE(b->a, 3);

    /* This deletes and then copy-constructs the wrapped value */
    Optional<MoveOnlyStruct> c;
    c = Utility::move(b);
    CORRADE_COMPARE(c->a, 3);
}

void OptionalTest::vectorOfMovableOptional() {
    std::vector<Optional<Movable>> vec;

    vec.emplace_back(Movable{23});
    vec.emplace_back();
    vec.emplace_back(NullOpt);
    vec.push_back(Movable{67});

    CORRADE_COMPARE(vec[0]->a, 23);
    CORRADE_VERIFY(!vec[1]);
    CORRADE_VERIFY(!vec[2]);
    CORRADE_COMPARE(vec[3]->a, 67);
}

void OptionalTest::constructConst() {
    {
        Optional<const Copyable> a{Copyable{32}};
        CORRADE_VERIFY(a);
        CORRADE_COMPARE(a->a, 32);

        Optional<const Copyable> b = a;
        CORRADE_COMPARE(b->a, 32);

        /* A const value can't be moved from, so it's copied */
        Optional<const Copyable> c = Utility::move(a);
        CORRADE_VERIFY(a);
        CORRADE_COMPARE(a->a, 32);
        CORRADE_COMPARE(c->a, 32);

        Copyable d = *Utility::move(c);
        CORRADE_COMPARE(d.a, 32);
    }

    /* Only the temporary got moved */
    CORRADE_COMPARE(Copyable::constructed, 5);
    CORRADE_COMPARE(Copyable::destructed, 5);
    CORRADE_COMPARE(Copyable::copied, 3);
    CORRADE_COMPARE(Copyable::moved, 1);

    CORRADE_VERIFY(std::is_same<decltype(std::declval<Optional<const Copyable>&>().operator->()), const Copyable*>::value);
    CORRADE_VERIFY(std::is_same<decltype(*std::declval<Optional<const Copyable>&>()), const Copyable&>::value);
    CORRADE_VERIFY(std::is_same<decltype(*std::declval<Optional<const Copyable>>()), Copyable>::value);

    CORRADE_VERIFY(std::is_nothrow_copy_constructible<Optional<const Copyable>>::value);
    CORRADE_VERIFY(std::is_nothrow_move_constructible<Optional<const Copyable>>::value);
    CORRADE_VERIFY(!std::is_copy_assignable<Optional<const Copyable>>::value);
    CORRADE_VERIFY(!std::is_move_assignable<Optional<const Copyable>>::value);
    CORRADE_VERIFY(!std::is_assignable<Optional<const Copyable>&, const Copyable&>::value);
    CORRADE_VERIFY(!std::is_copy_constructible<Optional<const Movable>>::value);
    CORRADE_VERIFY(!std::is_move_constructible<Optional<const Movable>>::value);
}

void OptionalTest::emplaceConst() {
    {
        Optional<const Copyable> a;
        a.emplace(32);
        CORRADE_COMPARE(a->a, 32);

        const Copyable& b = a.emplace(15);
        CORRADE_COMPARE(b.a, 15);
        CORRADE_VERIFY(&b == &*a);

        a = NullOpt;
        CORRADE_VERIFY(!a);
    }

    CORRADE_COMPARE(Copyable::constructed, 2);
    CORRADE_COMPARE(Copyable::destructed, 2);
    CORRADE_COMPARE(Copyable::copied, 0);
    CORRADE_COMPARE(Copyable::moved, 0);
}

constexpr int ReferencedValue = 47;

void OptionalTest::constructReference() {
    int x = 5;
    Optional<int&> a;
    Optional<int&> b = NullOpt;
    Optional<int&> c = x;
    Optional<int&> d{Corrade::InPlaceInit, x};
    CORRADE_VERIFY(!a);
    CORRADE_VERIFY(!b);
    CORRADE_VERIFY(c);
    CORRADE_VERIFY(d);
    CORRADE_VERIFY(&*c == &x);
    CORRADE_VERIFY(&*d == &x);
    CORRADE_VERIFY(c.operator->() == &x);

    Optional<int&> e = c;
    CORRADE_VERIFY(&*e == &x);

    CORRADE_VERIFY(std::is_same<Optional<int&>::Type, int&>::value);
    CORRADE_COMPARE(sizeof(Optional<int&>), sizeof(int*));
    #ifndef CORRADE_NO_STD_IS_TRIVIALLY_TRAITS
    CORRADE_VERIFY(std::is_trivially_copyable<Optional<const int&>>::value);
    #endif
    CORRADE_VERIFY(std::is_nothrow_default_constructible<Optional<int&>>::value);
    CORRADE_VERIFY(std::is_nothrow_constructible<Optional<int&>, int&>::value);
    CORRADE_VERIFY(std::is_nothrow_constructible<Optional<int&>, Corrade::InPlaceInitT, int&>::value);

    /* Binding a temporary would dangle */
    CORRADE_VERIFY(!std::is_constructible<Optional<const int&>, int>::value);
    CORRADE_VERIFY(!std::is_constructible<Optional<const int&>, Corrade::InPlaceInitT, int>::value);
    CORRADE_VERIFY(!std::is_constructible<Optional<int&>, const int&>::value);
}

void OptionalTest::constructReferenceConstexpr() {
    constexpr Optional<const int&> a;
    constexpr Optional<const int&> b = NullOpt;
    constexpr Optional<const int&> c = ReferencedValue;
    constexpr Optional<const int&> d{Corrade::InPlaceInit, ReferencedValue};
    constexpr Optional<const int&> e = c;
    constexpr Optional<const volatile int&> f = c;

    constexpr bool isASet = !!a;
    constexpr bool isCSet = !!c;
    constexpr const int* pointerC = c.operator->();
    constexpr const volatile int* pointerF = f.operator->();
    constexpr int valueD = *d;
    constexpr bool isEqual = c == d && c == e && c == 47 && 47 == c && a == b && a == NullOpt && NullOpt == a;
    constexpr bool isNotEqual = c != a && c != 46 && 46 != c && c != NullOpt && NullOpt != c && !(c != d);
    CORRADE_VERIFY(!isASet);
    CORRADE_VERIFY(isCSet);
    CORRADE_VERIFY(pointerC == &ReferencedValue);
    CORRADE_VERIFY(pointerF == &ReferencedValue);
    CORRADE_COMPARE(valueD, 47);
    CORRADE_VERIFY(isEqual);
    CORRADE_VERIFY(isNotEqual);
}

void OptionalTest::constructReferenceFromReference() {
    struct Base { int a; };
    struct Derived: Base {};

    int x = 5;
    Optional<int&> a = x;
    Optional<const int&> b = a;
    Optional<volatile int&> c = a;
    Optional<const volatile int&> d = b;
    CORRADE_VERIFY(&*b == &x);
    CORRADE_VERIFY(c.operator->() == &x);
    CORRADE_VERIFY(d.operator->() == &x);

    Optional<int&> empty;
    Optional<const int&> e = empty;
    CORRADE_VERIFY(!e);

    Derived derived;
    Optional<Derived&> f = derived;
    Optional<Base&> g = f;
    CORRADE_VERIFY(&*g == &derived);

    CORRADE_VERIFY(std::is_nothrow_constructible<Optional<const int&>, const Optional<int&>&>::value);
    CORRADE_VERIFY(!std::is_convertible<Optional<const int&>, Optional<int&>>::value);
    CORRADE_VERIFY(!std::is_convertible<Optional<volatile int&>, Optional<int&>>::value);
    CORRADE_VERIFY(!std::is_convertible<Optional<Base&>, Optional<Derived&>>::value);
    CORRADE_VERIFY(!std::is_convertible<Optional<int&>, Optional<float&>>::value);
}

void OptionalTest::constructReferenceFromValue() {
    struct Base { int a; };
    struct Derived: Base {};

    Optional<int> a{5};
    const Optional<int> ca{6};
    Optional<const int> constA{7};
    Optional<int> empty;

    Optional<int&> b = a;
    Optional<const int&> c = a;
    Optional<const int&> d = ca;
    Optional<const int&> e = constA;
    Optional<int&> f = empty;
    CORRADE_VERIFY(&*b == &*a);
    CORRADE_VERIFY(&*c == &*a);
    CORRADE_VERIFY(&*d == &*ca);
    CORRADE_VERIFY(&*e == &*constA);
    CORRADE_VERIFY(!f);

    *b = 15;
    CORRADE_COMPARE(*a, 15);

    Optional<Derived> derived{Corrade::InPlaceInit};
    Optional<Base&> g = derived;
    CORRADE_VERIFY(&*g == &*derived);

    CORRADE_VERIFY(std::is_nothrow_constructible<Optional<int&>, Optional<int>&>::value);
    CORRADE_VERIFY(std::is_nothrow_constructible<Optional<const int&>, const Optional<int>&>::value);
    CORRADE_VERIFY(!std::is_constructible<Optional<int&>, const Optional<int>&>::value);
    CORRADE_VERIFY(!std::is_constructible<Optional<int&>, Optional<const int>&>::value);
    CORRADE_VERIFY(!std::is_constructible<Optional<int&>, Optional<float>&>::value);

    /* The value of an expiring optional would dangle */
    CORRADE_VERIFY(!std::is_constructible<Optional<const int&>, Optional<int>>::value);
    CORRADE_VERIFY(!std::is_constructible<Optional<const int&>, const Optional<int>&&>::value);
}

void OptionalTest::assignReference() {
    int x = 5;
    int y = 6;
    Optional<int&> a = x;
    Optional<int&> b = y;

    /* Rebinds, keeping the values as they were */
    a = b;
    CORRADE_VERIFY(&*a == &y);
    CORRADE_COMPARE(x, 5);
    CORRADE_COMPARE(y, 6);

    a = x;
    CORRADE_VERIFY(&*a == &x);
    CORRADE_COMPARE(y, 6);

    Optional<int> c{7};
    a = c;
    CORRADE_VERIFY(&*a == &*c);

    a = NullOpt;
    CORRADE_VERIFY(!a);

    a = x;
    a = {};
    CORRADE_VERIFY(!a);

    CORRADE_VERIFY(std::is_nothrow_copy_assignable<Optional<int&>>::value);
    CORRADE_VERIFY(std::is_nothrow_assignable<Optional<int&>, NullOptT>::value);
    CORRADE_VERIFY(!std::is_assignable<Optional<const int&>&, int>::value);
    CORRADE_VERIFY(!std::is_assignable<Optional<const int&>&, Optional<int>>::value);
}

void OptionalTest::emplaceReference() {
    int x = 5;
    int y = 6;
    Optional<int&> a;

    int& b = a.emplace(x);
    CORRADE_VERIFY(&b == &x);
    CORRADE_VERIFY(&*a == &x);

    a.emplace(y);
    CORRADE_VERIFY(&*a == &y);
    CORRADE_COMPARE(x, 5);
}

void OptionalTest::compareReference() {
    int x = 5;
    int y = 5;
    int z = 6;
    Optional<const int&> a = x;
    Optional<const int&> b = y;
    Optional<const int&> c = z;
    Optional<const int&> empty;

    /* Values are compared, not addresses */
    CORRADE_VERIFY(a == b);
    CORRADE_VERIFY(!(a != b));
    CORRADE_VERIFY(a != c);
    CORRADE_VERIFY(a != empty);
    CORRADE_VERIFY(empty == Optional<const int&>{});

    CORRADE_VERIFY(a == 5);
    CORRADE_VERIFY(5 == a);
    CORRADE_VERIFY(a != 6);
    CORRADE_VERIFY(6 != a);
    CORRADE_VERIFY(empty != 5);
    CORRADE_VERIFY(5 != empty);

    CORRADE_VERIFY(a != NullOpt);
    CORRADE_VERIFY(NullOpt != a);
    CORRADE_VERIFY(empty == NullOpt);
    CORRADE_VERIFY(NullOpt == empty);
}

void OptionalTest::accessReference() {
    struct Foo { int a; };

    Foo foo{32};
    /* Constness of the optional doesn't affect the value */
    const Optional<Foo&> a = foo;
    a->a = 15;
    CORRADE_COMPARE(foo.a, 15);
    (*a).a = 7;
    CORRADE_COMPARE(foo.a, 7);

    CORRADE_VERIFY(std::is_same<decltype(*a), Foo&>::value);
    CORRADE_VERIFY(std::is_same<decltype(a.operator->()), Foo*>::value);
    CORRADE_VERIFY(std::is_same<decltype(*std::declval<Optional<const Foo&>>()), const Foo&>::value);
}

void OptionalTest::accessReferenceVolatile() {
    int x = 5;
    Optional<volatile int&> a = x;
    *a = 7;
    CORRADE_COMPARE(x, 7);
    CORRADE_VERIFY(a.operator->() == &x);
    CORRADE_VERIFY(a == 7);
    CORRADE_VERIFY(7 == a);
    CORRADE_VERIFY(a != 6);
    CORRADE_VERIFY(6 != a);

    const volatile int& cx = x;
    Optional<const volatile int&> b = cx;
    Optional<const volatile int&> c = a;
    CORRADE_VERIFY(b.operator->() == &x);
    CORRADE_VERIFY(b == 7);
    CORRADE_VERIFY(7 == b);
    CORRADE_VERIFY(b == c);
}

void OptionalTest::accessReferenceInvalid() {
    CORRADE_SKIP_IF_NO_DEBUG_ASSERT();

    Optional<int&> a;
    CORRADE_VERIFY(!a);

    Containers::String out;
    {
        Error redirectError{&out};
        a.operator->();
        *a;
    }
    CORRADE_COMPARE(out,
        "Containers::Optional: the optional is empty\n"
        "Containers::Optional: the optional is empty\n");
}

void OptionalTest::debugReference() {
    int x = 42;
    Containers::String out;
    Debug{&out} << Optional<int&>{x} << Optional<const int&>{} << Optional<const volatile int&>{x};
    CORRADE_COMPARE(out, "42 Containers::NullOpt 42\n");
}

}}}}

CORRADE_TEST_MAIN(Corrade::Containers::Test::OptionalTest)
