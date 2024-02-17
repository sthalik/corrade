/*
    This file is part of Corrade.

    Copyright © 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016,
                2017, 2018, 2019, 2020, 2021, 2022, 2023
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

#include "Corrade/Containers/StaticArray.h"
#include "Corrade/TestSuite/Tester.h"

namespace {

struct IntView5 {
    explicit IntView5(int* data): data{data} {}

    int* data;
};

struct ConstIntView5 {
    explicit ConstIntView5(const int* data): data{data} {}

    const int* data;
};

}

namespace Corrade { namespace Containers {

namespace Implementation {

template<> struct StaticArrayViewConverter<5, int, IntView5> {
    static IntView5 to(StaticArrayView<5, int> other) {
        return IntView5{other.data()};
    }
};

template<> struct StaticArrayViewConverter<5, const int, ConstIntView5> {
    static ConstIntView5 to(StaticArrayView<5, const int> other) {
        return ConstIntView5{other.data()};
    }
};

}

namespace Test { namespace {

struct StaticArrayTest: TestSuite::Tester {
    explicit StaticArrayTest();

    void resetCounters();

    void constructValueInit();
    void constructDefaultInit();
    void constructNoInit();
    void constructInPlaceInit();
    void constructInPlaceInitOneArgument();
    void constructInPlaceInitMoveOnly();
    void constructDirectInit();
    void constructDirectInitMoveOnly();
    void constructImmovable();
    void constructNoImplicitConstructor();
    void constructDirectReferences();
    void constructArray();
    void constructArrayRvalue();
    void constructArrayMove();

    /* No constructZeroNullPointerAmbiguity() here as the StaticArray is never
       empty, thus never null, thus std::nullptr_t constructor makes no sense */

    void copy();
    void move();

    void convertBool();
    void convertPointer();
    void convertView();
    void convertViewDerived();
    void convertViewOverload();
    void convertStaticView();
    void convertStaticViewDerived();
    void convertStaticViewOverload();
    void convertVoid();
    void convertConstVoid();
    void convertToExternalView();
    void convertToConstExternalView();

    void access();
    void accessConst();
    void rvalueArrayAccess();
    void rangeBasedFor();

    void slice();
    void slicePointer();
    void sliceToStatic();
    void sliceToStaticPointer();
    void sliceZeroNullPointerAmbiguity();

    void cast();
    void size();

    void constructorExplicitInCopyInitialization();
    void copyConstructPlainStruct();
    void moveConstructPlainStruct();
};

StaticArrayTest::StaticArrayTest() {
    addTests({&StaticArrayTest::constructValueInit,
              &StaticArrayTest::constructDefaultInit});

    addTests({&StaticArrayTest::constructNoInit},
        &StaticArrayTest::resetCounters, &StaticArrayTest::resetCounters);

    addTests({&StaticArrayTest::constructInPlaceInit,
              &StaticArrayTest::constructInPlaceInitOneArgument});

    addTests({&StaticArrayTest::constructInPlaceInitMoveOnly},
        &StaticArrayTest::resetCounters, &StaticArrayTest::resetCounters);

    addTests({&StaticArrayTest::constructDirectInit});

    addTests({&StaticArrayTest::constructDirectInitMoveOnly,
              &StaticArrayTest::constructImmovable},
        &StaticArrayTest::resetCounters, &StaticArrayTest::resetCounters);

    addTests({&StaticArrayTest::constructNoImplicitConstructor,
              &StaticArrayTest::constructDirectReferences,
              &StaticArrayTest::constructArray,
              &StaticArrayTest::constructArrayRvalue,
              &StaticArrayTest::constructArrayMove});

    addTests({&StaticArrayTest::copy,
              &StaticArrayTest::move},
        &StaticArrayTest::resetCounters, &StaticArrayTest::resetCounters);

    addTests({&StaticArrayTest::convertBool,
              &StaticArrayTest::convertPointer,
              &StaticArrayTest::convertView,
              &StaticArrayTest::convertViewDerived,
              &StaticArrayTest::convertViewOverload,
              &StaticArrayTest::convertStaticView,
              &StaticArrayTest::convertStaticViewDerived,
              &StaticArrayTest::convertStaticViewOverload,
              &StaticArrayTest::convertVoid,
              &StaticArrayTest::convertConstVoid,
              &StaticArrayTest::convertToExternalView,
              &StaticArrayTest::convertToConstExternalView,

              &StaticArrayTest::access,
              &StaticArrayTest::accessConst,
              &StaticArrayTest::rvalueArrayAccess,
              &StaticArrayTest::rangeBasedFor,

              &StaticArrayTest::slice,
              &StaticArrayTest::slicePointer,
              &StaticArrayTest::sliceToStatic,
              &StaticArrayTest::sliceToStaticPointer,
              &StaticArrayTest::sliceZeroNullPointerAmbiguity,

              &StaticArrayTest::cast,
              &StaticArrayTest::size,

              &StaticArrayTest::constructorExplicitInCopyInitialization,
              &StaticArrayTest::copyConstructPlainStruct,
              &StaticArrayTest::moveConstructPlainStruct});
}

void StaticArrayTest::constructValueInit() {
    const StaticArray<5, int> a1;
    const StaticArray<5, int> a2{Corrade::ValueInit};
    CORRADE_VERIFY(a1);
    CORRADE_VERIFY(a2);
    CORRADE_VERIFY(!a1.isEmpty());
    CORRADE_VERIFY(!a2.isEmpty());
    CORRADE_COMPARE(a1.size(), (StaticArray<5, int>::Size));
    CORRADE_COMPARE(a2.size(), (StaticArray<5, int>::Size));
    CORRADE_COMPARE(a1.size(), 5);
    CORRADE_COMPARE(a2.size(), 5);

    /* Values should be zero-initialized (same as ValueInit) */
    CORRADE_COMPARE(a1[0], 0);
    CORRADE_COMPARE(a2[0], 0);
    CORRADE_COMPARE(a1[1], 0);
    CORRADE_COMPARE(a2[1], 0);
    CORRADE_COMPARE(a1[2], 0);
    CORRADE_COMPARE(a2[2], 0);
    CORRADE_COMPARE(a1[3], 0);
    CORRADE_COMPARE(a2[3], 0);
    CORRADE_COMPARE(a1[4], 0);
    CORRADE_COMPARE(a2[4], 0);

    /* Implicit construction is not allowed */
    CORRADE_VERIFY(!std::is_convertible<Corrade::ValueInitT, StaticArray<5, int>>::value);
}

void StaticArrayTest::constructDefaultInit() {
    const StaticArray<5, int> a{Corrade::DefaultInit};
    CORRADE_VERIFY(a);

    /* Values are random memory */

    /* Implicit construction is not allowed */
    CORRADE_VERIFY(!std::is_convertible<Corrade::DefaultInitT, StaticArray<5, int>>::value);
}

struct Throwable {
    /* Clang complains this function is unused. But removing it may have
       unintended consequences, so don't. */
    explicit Throwable(int) CORRADE_UNUSED {}
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

    /*implicit*/ Copyable(int a = 0) noexcept: a{a} { ++constructed; }
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
    /* Clang complains this function is unused. But removing it may have
       unintended consequences, so don't. */
    CORRADE_UNUSED Copyable& operator=(Copyable&& other) noexcept {
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

    /*implicit*/ Movable(int a = 0) noexcept: a{a} { ++constructed; }
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

void swap(Movable& a, Movable& b) {
    /* Swap these without copying the parent class */
    Corrade::Utility::swap(a.a, b.a);
}

struct Immovable {
    static int constructed;
    static int destructed;

    Immovable(const Immovable&) = delete;
    Immovable(Immovable&&) = delete;
    /*implicit*/ Immovable(int a = 0) noexcept: a{a} { ++constructed; }
    ~Immovable() { ++destructed; }
    Immovable& operator=(const Immovable&) = delete;
    Immovable& operator=(Immovable&&) = delete;

    int a;
};

int Immovable::constructed = 0;
int Immovable::destructed = 0;

void StaticArrayTest::resetCounters() {
    Copyable::constructed = Copyable::destructed = Copyable::copied = Copyable::moved =
        Movable::constructed = Movable::destructed = Movable::moved =
            Immovable::constructed = Immovable::destructed = 0;
}

void StaticArrayTest::constructNoInit() {
    {
        StaticArray<3, Copyable> a{Corrade::InPlaceInit, 57, 39, 78};
        CORRADE_COMPARE(Copyable::constructed, 3);
        CORRADE_COMPARE(Copyable::destructed, 0);
        CORRADE_COMPARE(Copyable::copied, 0);
        CORRADE_COMPARE(Copyable::moved, 0);

        new(&a) StaticArray<3, Copyable>{Corrade::NoInit};
        CORRADE_COMPARE(Copyable::constructed, 3);
        CORRADE_COMPARE(Copyable::destructed, 0);
        CORRADE_COMPARE(Copyable::copied, 0);
        CORRADE_COMPARE(Copyable::moved, 0);
    }

    CORRADE_COMPARE(Copyable::constructed, 3);
    CORRADE_COMPARE(Copyable::destructed, 3);
    CORRADE_COMPARE(Copyable::copied, 0);
    CORRADE_COMPARE(Copyable::moved, 0);

    /* Implicit construction is not allowed */
    CORRADE_VERIFY(!std::is_convertible<Corrade::NoInitT, StaticArray<5, Copyable>>::value);
}

void StaticArrayTest::constructInPlaceInit() {
    const StaticArray<5, int> a{1, 2, 3, 4, 5};
    const StaticArray<5, int> b{Corrade::InPlaceInit, 1, 2, 3, 4, 5};

    CORRADE_COMPARE(a[0], 1);
    CORRADE_COMPARE(b[0], 1);
    CORRADE_COMPARE(a[1], 2);
    CORRADE_COMPARE(b[1], 2);
    CORRADE_COMPARE(a[2], 3);
    CORRADE_COMPARE(b[2], 3);
    CORRADE_COMPARE(a[3], 4);
    CORRADE_COMPARE(b[3], 4);
    CORRADE_COMPARE(a[4], 5);
    CORRADE_COMPARE(b[4], 5);
}

void StaticArrayTest::constructInPlaceInitOneArgument() {
    const StaticArray<1, int> a{17};
    CORRADE_COMPARE(a[0], 17);
}

void StaticArrayTest::constructInPlaceInitMoveOnly() {
    {
        const StaticArray<3, Movable> a{Movable{1}, Movable{2}, Movable{3}};
        const StaticArray<3, Movable> b{Corrade::InPlaceInit, Movable{1}, Movable{2}, Movable{3}};

        CORRADE_COMPARE(a[0].a, 1);
        CORRADE_COMPARE(b[0].a, 1);
        CORRADE_COMPARE(a[1].a, 2);
        CORRADE_COMPARE(b[1].a, 2);
        CORRADE_COMPARE(a[2].a, 3);
        CORRADE_COMPARE(b[2].a, 3);

        /* 6 temporaries that were moved to the concrete places 6 times */
        CORRADE_COMPARE(Movable::constructed, 6 + 6);
        CORRADE_COMPARE(Movable::destructed, 6);
        CORRADE_COMPARE(Movable::moved, 6);
    }

    CORRADE_COMPARE(Movable::constructed, 6 + 6);
    CORRADE_COMPARE(Movable::destructed, 6 + 6);
    CORRADE_COMPARE(Movable::moved, 6);
}

void StaticArrayTest::constructDirectInit() {
    const StaticArray<5, int> a{Corrade::DirectInit, -37};
    CORRADE_COMPARE(a[0], -37);
    CORRADE_COMPARE(a[1], -37);
    CORRADE_COMPARE(a[2], -37);
    CORRADE_COMPARE(a[3], -37);
    CORRADE_COMPARE(a[4], -37);
}

void StaticArrayTest::constructDirectInitMoveOnly() {
    {
        /* This one is weird as it moves one argument 3 times, but should work
           nevertheless */
        const StaticArray<3, Movable> a{Corrade::DirectInit, Movable{-37}};
        CORRADE_COMPARE(a[0].a, -37);
        CORRADE_COMPARE(a[1].a, -37);
        CORRADE_COMPARE(a[2].a, -37);

        /* 1 temporary that was moved to the concrete places 3 times */
        CORRADE_COMPARE(Movable::constructed, 1 + 3);
        CORRADE_COMPARE(Movable::destructed, 1);
        CORRADE_COMPARE(Movable::moved, 3);
    }

    CORRADE_COMPARE(Movable::constructed, 1 + 3);
    CORRADE_COMPARE(Movable::destructed, 1 + 3);
    CORRADE_COMPARE(Movable::moved, 3);
}

void StaticArrayTest::constructImmovable() {
    /* Can't use ValueInit because that apparently copy-constructs the array
       elements (huh?) */
    const StaticArray<5, Immovable> a{Corrade::DefaultInit};
    CORRADE_VERIFY(a);
}

void StaticArrayTest::constructNoImplicitConstructor() {
    struct NoImplicitConstructor {
        NoImplicitConstructor(int i): i{i} {}

        int i;
    };

    const StaticArray<5, NoImplicitConstructor> a{Corrade::DirectInit, 5};
    CORRADE_VERIFY(a);
    CORRADE_COMPARE(a[0].i, 5);
    CORRADE_COMPARE(a[1].i, 5);
    CORRADE_COMPARE(a[2].i, 5);
    CORRADE_COMPARE(a[3].i, 5);
    CORRADE_COMPARE(a[4].i, 5);

    const StaticArray<5, NoImplicitConstructor> b{Corrade::InPlaceInit, 1, 2, 3, 4, 5};
    CORRADE_VERIFY(b);
    CORRADE_COMPARE(b[0].i, 1);
    CORRADE_COMPARE(b[1].i, 2);
    CORRADE_COMPARE(b[2].i, 3);
    CORRADE_COMPARE(b[3].i, 4);
    CORRADE_COMPARE(b[4].i, 5);
}

void StaticArrayTest::constructDirectReferences() {
    struct NonCopyable {
        NonCopyable(const NonCopyable&) = delete;
        NonCopyable(NonCopyable&&) = delete;
        NonCopyable& operator=(const NonCopyable&) = delete;
        NonCopyable& operator=(NonCopyable&&) = delete;
        NonCopyable() = default;
    } a;

    struct Reference {
        Reference(NonCopyable&) {}
    };

    const StaticArray<5, Reference> b{Corrade::DirectInit, a};
    CORRADE_VERIFY(b);
}

void StaticArrayTest::constructArray() {
    struct PairOfInts {
        int a, b;
    };

    const PairOfInts data[]{
        {1, 2},
        {3, 4},
        {5, 6}
    };
    StaticArray<3, PairOfInts> a1{data};
    StaticArray<3, PairOfInts> a2{Corrade::InPlaceInit, data};
    CORRADE_COMPARE(a1[0].a, 1);
    CORRADE_COMPARE(a2[0].a, 1);
    CORRADE_COMPARE(a1[0].b, 2);
    CORRADE_COMPARE(a2[0].b, 2);
    CORRADE_COMPARE(a1[1].a, 3);
    CORRADE_COMPARE(a2[1].a, 3);
    CORRADE_COMPARE(a1[1].b, 4);
    CORRADE_COMPARE(a2[1].b, 4);
    CORRADE_COMPARE(a1[2].a, 5);
    CORRADE_COMPARE(a2[2].a, 5);
    CORRADE_COMPARE(a1[2].b, 6);
    CORRADE_COMPARE(a2[2].b, 6);
}

void StaticArrayTest::constructArrayRvalue() {
    struct PairOfInts {
        int a, b;
    };

    StaticArray<3, PairOfInts> a1{{
        {1, 2},
        {3, 4},
        {5, 6}
    }};
    StaticArray<3, PairOfInts> a2{Corrade::InPlaceInit, {
        {1, 2},
        {3, 4},
        {5, 6}
    }};
    CORRADE_COMPARE(a1[0].a, 1);
    CORRADE_COMPARE(a2[0].a, 1);
    CORRADE_COMPARE(a1[0].b, 2);
    CORRADE_COMPARE(a2[0].b, 2);
    CORRADE_COMPARE(a1[1].a, 3);
    CORRADE_COMPARE(a2[1].a, 3);
    CORRADE_COMPARE(a1[1].b, 4);
    CORRADE_COMPARE(a2[1].b, 4);
    CORRADE_COMPARE(a1[2].a, 5);
    CORRADE_COMPARE(a2[2].a, 5);
    CORRADE_COMPARE(a1[2].b, 6);
    CORRADE_COMPARE(a2[2].b, 6);
}

void StaticArrayTest::constructArrayMove() {
    #ifdef CORRADE_MSVC2017_COMPATIBILITY
    /* MSVC 2015 and 2017 fails with
        error C2440: 'return': cannot convert from 'T [3]' to 'T (&&)[3]'
        Corrade/Utility/Move.h(88): note: You cannot bind an lvalue to an rvalue reference
       on the Utility::move() call inside the r-value constructor, std::move()
       behaves the same. Because of that, only the copying constructor can be
       enabled, as otherwise it would pick it for the constructArrayRvalue()
       above as well and fail even in the case where nothing needs to be
       moved. */
    CORRADE_SKIP("MSVC 2015 and 2017 isn't able to move arrays.");
    #else
    struct MovableInt {
        Movable a;
        int b;
    };

    {
        StaticArray<3, MovableInt> a1{{
            {Movable{1}, 2},
            {Movable{3}, 4},
            {Movable{5}, 6}
        }};
        StaticArray<3, MovableInt> a2{Corrade::InPlaceInit, {
            {Movable{1}, 2},
            {Movable{3}, 4},
            {Movable{5}, 6}
        }};
        CORRADE_COMPARE(a1[0].a.a, 1);
        CORRADE_COMPARE(a2[0].a.a, 1);
        CORRADE_COMPARE(a1[0].b, 2);
        CORRADE_COMPARE(a2[0].b, 2);
        CORRADE_COMPARE(a1[1].a.a, 3);
        CORRADE_COMPARE(a2[1].a.a, 3);
        CORRADE_COMPARE(a1[1].b, 4);
        CORRADE_COMPARE(a2[1].b, 4);
        CORRADE_COMPARE(a1[2].a.a, 5);
        CORRADE_COMPARE(a2[2].a.a, 5);
        CORRADE_COMPARE(a1[2].b, 6);
        CORRADE_COMPARE(a2[2].b, 6);

        /* 6 temporaries that were moved to the concrete places 6 times */
        CORRADE_COMPARE(Movable::constructed, 6 + 6);
        CORRADE_COMPARE(Movable::destructed, 6);
        CORRADE_COMPARE(Movable::moved, 6);
    }

    CORRADE_COMPARE(Movable::constructed, 6 + 6);
    CORRADE_COMPARE(Movable::destructed, 6 + 6);
    CORRADE_COMPARE(Movable::moved, 6);
    #endif
}

void StaticArrayTest::copy() {
    {
        StaticArray<3, Copyable> a{Corrade::InPlaceInit, 1, 2, 3};

        StaticArray<3, Copyable> b{a};
        CORRADE_COMPARE(b[0].a, 1);
        CORRADE_COMPARE(b[1].a, 2);
        CORRADE_COMPARE(b[2].a, 3);

        StaticArray<3, Copyable> c;
        c = b;
        CORRADE_COMPARE(c[0].a, 1);
        CORRADE_COMPARE(c[1].a, 2);
        CORRADE_COMPARE(c[2].a, 3);
    }

    CORRADE_COMPARE(Copyable::constructed, 9);
    CORRADE_COMPARE(Copyable::destructed, 9);
    CORRADE_COMPARE(Copyable::copied, 6);
    CORRADE_COMPARE(Copyable::moved, 0);

    CORRADE_VERIFY(std::is_nothrow_copy_constructible<Copyable>::value);
    CORRADE_VERIFY(std::is_nothrow_copy_constructible<StaticArray<3, Copyable>>::value);
    CORRADE_VERIFY(std::is_nothrow_copy_assignable<Copyable>::value);
    CORRADE_VERIFY(std::is_nothrow_copy_assignable<StaticArray<3, Copyable>>::value);

    CORRADE_VERIFY(std::is_copy_constructible<Throwable>::value);
    CORRADE_VERIFY(!std::is_nothrow_copy_constructible<Throwable>::value);
    CORRADE_VERIFY(!std::is_nothrow_copy_constructible<StaticArray<3, Throwable>>::value);
    CORRADE_VERIFY(std::is_copy_assignable<Throwable>::value);
    CORRADE_VERIFY(!std::is_nothrow_copy_assignable<Throwable>::value);
    CORRADE_VERIFY(!std::is_nothrow_copy_assignable<StaticArray<3, Throwable>>::value);
}

void StaticArrayTest::move() {
    {
        StaticArray<3, Movable> a{Corrade::InPlaceInit, 1, 2, 3};

        StaticArray<3, Movable> b{Utility::move(a)};
        CORRADE_COMPARE(b[0].a, 1);
        CORRADE_COMPARE(b[1].a, 2);
        CORRADE_COMPARE(b[2].a, 3);

        StaticArray<3, Movable> c;
        c = Utility::move(b); /* this uses the swap() specialization -> no move */
        CORRADE_COMPARE(c[0].a, 1);
        CORRADE_COMPARE(c[1].a, 2);
        CORRADE_COMPARE(c[2].a, 3);
    }

    CORRADE_COMPARE(Movable::constructed, 9);
    CORRADE_COMPARE(Movable::destructed, 9);
    CORRADE_COMPARE(Movable::moved, 3);

    CORRADE_VERIFY(!std::is_copy_constructible<Movable>::value);
    CORRADE_VERIFY(!std::is_copy_assignable<Movable>::value);
    {
        CORRADE_EXPECT_FAIL("StaticArray currently doesn't propagate deleted copy constructor/assignment correctly.");
        CORRADE_VERIFY(!std::is_copy_constructible<StaticArray<3, Movable>>::value);
        CORRADE_VERIFY(!std::is_copy_assignable<StaticArray<3, Movable>>::value);
    }

    CORRADE_VERIFY(std::is_move_constructible<StaticArray<3, Movable>>::value);
    CORRADE_VERIFY(std::is_nothrow_move_constructible<Movable>::value);
    CORRADE_VERIFY(std::is_nothrow_move_constructible<StaticArray<3, Movable>>::value);
    CORRADE_VERIFY(std::is_move_assignable<StaticArray<3, Movable>>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<Movable>::value);
    CORRADE_VERIFY(std::is_nothrow_move_assignable<StaticArray<3, Movable>>::value);

    CORRADE_VERIFY(std::is_move_constructible<Throwable>::value);
    CORRADE_VERIFY(!std::is_nothrow_move_constructible<Throwable>::value);
    CORRADE_VERIFY(!std::is_nothrow_move_constructible<StaticArray<3, Throwable>>::value);
    CORRADE_VERIFY(std::is_move_assignable<Throwable>::value);
    CORRADE_VERIFY(!std::is_nothrow_move_assignable<Throwable>::value);
    CORRADE_VERIFY(!std::is_nothrow_move_assignable<StaticArray<3, Throwable>>::value);
}

void StaticArrayTest::convertBool() {
    CORRADE_VERIFY(StaticArray<5, int>{});

    /* Explicit conversion to bool is allowed, but not to int */
    CORRADE_VERIFY(std::is_constructible<bool, StaticArray<5, int>>::value);
    CORRADE_VERIFY(!std::is_constructible<int, StaticArray<5, int>>::value);
}

void StaticArrayTest::convertPointer() {
    StaticArray<5, int> a;
    int* b = a;
    CORRADE_COMPARE(b, a.begin());

    const StaticArray<5, int> c;
    const int* d = c;
    CORRADE_COMPARE(d, c.begin());

    /* Pointer arithmetic */
    const StaticArray<5, int> e;
    const int* f = e + 2;
    CORRADE_COMPARE(f, &e[2]);

    /* Verify that we can't convert rvalues. Not using is_convertible to catch
       also accidental explicit conversions. */
    CORRADE_VERIFY(std::is_constructible<int*, StaticArray<5, int>&>::value);
    CORRADE_VERIFY(std::is_constructible<const int*, const StaticArray<5, int>&>::value);
    CORRADE_VERIFY(!std::is_constructible<int*, StaticArray<5, int>>::value);
    CORRADE_VERIFY(!std::is_constructible<int*, StaticArray<5, int>&&>::value);

    /* Deleting const&& overload and leaving only const& one will not, in fact,
       disable conversion of const Array&& to pointer, but rather make the
       conversion ambiguous, which is not what we want, as it breaks e.g.
       rvalueArrayAccess() test. Not using is_convertible to catch also
       accidental explicit conversions. */
    {
        CORRADE_EXPECT_FAIL("I don't know how to properly disable conversion of const Array&& to pointer.");
        CORRADE_VERIFY(!std::is_constructible<const int*, const StaticArray<5, int>>::value);
        CORRADE_VERIFY(!std::is_constructible<const int*, const StaticArray<5, int>&&>::value);
    }
}

void StaticArrayTest::convertView() {
    StaticArray<5, int> a;
    const StaticArray<5, int> ca;
    StaticArray<5, const int> ac;
    const StaticArray<5, const int> cac;

    {
        const ArrayView<int> b = a;
        const ArrayView<const int> cb = ca;
        const ArrayView<const int> bc = ac;
        const ArrayView<const int> cbc = cac;
        CORRADE_VERIFY(b.begin() == a.begin());
        CORRADE_VERIFY(bc.begin() == ac.begin());
        CORRADE_VERIFY(cb.begin() == ca.begin());
        CORRADE_VERIFY(cbc.begin() == cac.begin());
        CORRADE_COMPARE(b.size(), 5);
        CORRADE_COMPARE(cb.size(), 5);
        CORRADE_COMPARE(bc.size(), 5);
        CORRADE_COMPARE(cbc.size(), 5);
    } {
        const auto b = arrayView(a);
        const auto cb = arrayView(ca);
        const auto bc = arrayView(ac);
        const auto cbc = arrayView(cac);
        CORRADE_VERIFY(std::is_same<decltype(b), const ArrayView<int>>::value);
        CORRADE_VERIFY(std::is_same<decltype(cb), const ArrayView<const int>>::value);
        CORRADE_VERIFY(std::is_same<decltype(bc), const ArrayView<const int>>::value);
        CORRADE_VERIFY(std::is_same<decltype(cbc), const ArrayView<const int>>::value);
        CORRADE_VERIFY(b.begin() == a.begin());
        CORRADE_VERIFY(bc.begin() == ac.begin());
        CORRADE_VERIFY(cb.begin() == ca.begin());
        CORRADE_VERIFY(cbc.begin() == cac.begin());
        CORRADE_COMPARE(b.size(), 5);
        CORRADE_COMPARE(cb.size(), 5);
        CORRADE_COMPARE(bc.size(), 5);
        CORRADE_COMPARE(cbc.size(), 5);
    }
}

void StaticArrayTest::convertViewDerived() {
    struct A { int i; };
    struct B: A {};

    /* Valid use case: constructing Containers::ArrayView<Math::Vector<3, Float>>
       from Containers::ArrayView<Color3> because the data have the same size
       and data layout */

    StaticArray<5, B> b;
    ArrayView<A> a = b;

    CORRADE_VERIFY(a == b);
    CORRADE_COMPARE(a.size(), 5);
}

bool takesAView(ArrayView<int>) { return true; }
bool takesAConstView(ArrayView<const int>) { return true; }
CORRADE_UNUSED bool takesAView(ArrayView<float>) { return false; }
CORRADE_UNUSED bool takesAConstView(ArrayView<const float>) { return false; }

void StaticArrayTest::convertViewOverload() {
    StaticArray<5, int> a;
    const StaticArray<5, int> ca;

    /* It should pick the correct one and not fail, assert or be ambiguous */
    CORRADE_VERIFY(takesAView(a));
    CORRADE_VERIFY(takesAConstView(a));
    CORRADE_VERIFY(takesAConstView(ca));
}

void StaticArrayTest::convertStaticView() {
    StaticArray<5, int> a;
    const StaticArray<5, int> ca;
    StaticArray<5, const int> ac;
    const StaticArray<5, const int> cac;

    {
        const StaticArrayView<5, int> b = a;
        const StaticArrayView<5, const int> cb = ca;
        const StaticArrayView<5, const int> bc = ac;
        const StaticArrayView<5, const int> cbc = cac;
        CORRADE_VERIFY(b.begin() == a.begin());
        CORRADE_VERIFY(bc.begin() == ac.begin());
        CORRADE_VERIFY(cb.begin() == ca.begin());
        CORRADE_VERIFY(cbc.begin() == cac.begin());
        CORRADE_COMPARE(b.size(), 5);
        CORRADE_COMPARE(cb.size(), 5);
        CORRADE_COMPARE(bc.size(), 5);
        CORRADE_COMPARE(cbc.size(), 5);
    } {
        const auto b = staticArrayView(a);
        const auto cb = staticArrayView(ca);
        const auto bc = staticArrayView(ac);
        const auto cbc = staticArrayView(cac);
        CORRADE_VERIFY(std::is_same<decltype(b), const StaticArrayView<5, int>>::value);
        CORRADE_VERIFY(std::is_same<decltype(cb), const StaticArrayView<5, const int>>::value);
        CORRADE_VERIFY(std::is_same<decltype(bc), const StaticArrayView<5, const int>>::value);
        CORRADE_VERIFY(std::is_same<decltype(cbc), const StaticArrayView<5, const int>>::value);
        CORRADE_VERIFY(b.begin() == a.begin());
        CORRADE_VERIFY(bc.begin() == ac.begin());
        CORRADE_VERIFY(cb.begin() == ca.begin());
        CORRADE_VERIFY(cbc.begin() == cac.begin());
        CORRADE_COMPARE(b.size(), 5);
        CORRADE_COMPARE(cb.size(), 5);
        CORRADE_COMPARE(bc.size(), 5);
        CORRADE_COMPARE(cbc.size(), 5);
    }
}

void StaticArrayTest::convertStaticViewDerived() {
    struct A { int i; };
    struct B: A {};

    /* Valid use case: constructing Containers::ArrayView<Math::Vector<3, Float>>
       from Containers::ArrayView<Color3> because the data have the same size
       and data layout */

    StaticArray<5, B> b;
    StaticArrayView<5, A> a = b;

    CORRADE_VERIFY(a == b);
    CORRADE_COMPARE(a.size(), 5);
}

bool takesAStaticView(StaticArrayView<5, int>) { return true; }
bool takesAStaticConstView(StaticArrayView<5, const int>) { return true; }
CORRADE_UNUSED bool takesAStaticView(StaticArrayView<5, float>) { return false; }
CORRADE_UNUSED bool takesAStaticConstView(StaticArrayView<5, const float>) { return false; }

void StaticArrayTest::convertStaticViewOverload() {
    StaticArray<5, int> a;
    const StaticArray<5, int> ca;

    /* It should pick the correct one and not fail, assert or be ambiguous */
    CORRADE_VERIFY(takesAStaticView(a));
    CORRADE_VERIFY(takesAStaticConstView(a));
    CORRADE_VERIFY(takesAStaticConstView(ca));
}

void StaticArrayTest::convertVoid() {
    StaticArray<5, int> a;
    ArrayView<void> b = a;
    CORRADE_VERIFY(b == a);
    CORRADE_COMPARE(b.size(), 5*sizeof(int));
}

void StaticArrayTest::convertConstVoid() {
    StaticArray<5, int> a;
    const StaticArray<5, int> ca;
    ArrayView<const void> b = a;
    ArrayView<const void> cb = ca;
    CORRADE_VERIFY(b == a);
    CORRADE_VERIFY(cb == ca);
    CORRADE_COMPARE(b.size(), 5*sizeof(int));
    CORRADE_COMPARE(cb.size(), 5*sizeof(int));
}

void StaticArrayTest::convertToExternalView() {
    StaticArray<5, int> a{1, 2, 3, 4, 5};

    IntView5 b = a;
    CORRADE_COMPARE(b.data, a.data());

    ConstIntView5 cb = a;
    CORRADE_COMPARE(cb.data, a.data());

    /* Conversion to a different size or type is not allowed */
    /** @todo For some reason I can't use is_constructible here because it
       doesn't consider conversion operators in this case. In others (such as
       with ArrayView) it does, why? */
    CORRADE_VERIFY(std::is_convertible<StaticArray<5, int>, IntView5>::value);
    CORRADE_VERIFY(std::is_convertible<StaticArray<5, int>, ConstIntView5>::value);
    CORRADE_VERIFY(!std::is_convertible<StaticArray<6, int>, IntView5>::value);
    CORRADE_VERIFY(!std::is_convertible<StaticArray<6, int>, ConstIntView5>::value);
    CORRADE_VERIFY(!std::is_convertible<StaticArray<5, float>, IntView5>::value);
    CORRADE_VERIFY(!std::is_convertible<StaticArray<5, float>, ConstIntView5>::value);
}

void StaticArrayTest::convertToConstExternalView() {
    const StaticArray<5, int> a{1, 2, 3, 4, 5};

    ConstIntView5 b = a;
    CORRADE_COMPARE(b.data, a.data());

    /* Conversion to a different size or type is not allowed */
    /** @todo For some reason I can't use is_constructible here because it
       doesn't consider conversion operators in this case. In others (such as
       with ArrayView) it does, why? */
    CORRADE_VERIFY(std::is_convertible<const StaticArray<5, int>, ConstIntView5>::value);
    CORRADE_VERIFY(!std::is_convertible<const StaticArray<6, int>, ConstIntView5>::value);
    CORRADE_VERIFY(!std::is_convertible<const StaticArray<5, float>, ConstIntView5>::value);
}

void StaticArrayTest::access() {
    StaticArray<5, int> a;
    for(std::size_t i = 0; i != 5; ++i)
        a[i] = i;

    CORRADE_COMPARE(a.data(), static_cast<int*>(a));
    CORRADE_COMPARE(a.front(), 0);
    CORRADE_COMPARE(a.back(), 4);
    CORRADE_COMPARE(*(a.begin()+2), 2);
    CORRADE_COMPARE(a[4], 4);
    CORRADE_COMPARE(a.end()-a.begin(), 5);
    CORRADE_COMPARE(a.cbegin(), a.begin());
    CORRADE_COMPARE(a.cend(), a.end());
}

void StaticArrayTest::accessConst() {
    StaticArray<5, int> a;
    for(std::size_t i = 0; i != 5; ++i)
        a[i] = i;

    const StaticArray<5, int>& ca = a;
    CORRADE_COMPARE(ca.data(), static_cast<int*>(a));
    CORRADE_COMPARE(ca.front(), 0);
    CORRADE_COMPARE(ca.back(), 4);
    CORRADE_COMPARE(*(ca.begin()+2), 2);
    CORRADE_COMPARE(ca[4], 4);
    CORRADE_COMPARE(ca.end() - ca.begin(), 5);
    CORRADE_COMPARE(ca.cbegin(), ca.begin());
    CORRADE_COMPARE(ca.cend(), ca.end());
}

void StaticArrayTest::rvalueArrayAccess() {
    CORRADE_COMPARE((StaticArray<5, int>{Corrade::DirectInit, 3})[2], 3);
}

void StaticArrayTest::rangeBasedFor() {
    StaticArray<5, int> a;
    for(auto& i: a)
        i = 3;

    CORRADE_COMPARE(a[0], 3);
    CORRADE_COMPARE(a[1], 3);
    CORRADE_COMPARE(a[2], 3);
    CORRADE_COMPARE(a[3], 3);
    CORRADE_COMPARE(a[4], 3);

    /* To verify the constant begin()/end() accessors */
    const StaticArray<5, int>& ca = a;
    for(auto&& i: ca)
        CORRADE_COMPARE(i, 3);
}

void StaticArrayTest::slice() {
    StaticArray<5, int> a{Corrade::InPlaceInit, 1, 2, 3, 4, 5};
    const StaticArray<5, int> ac{Corrade::InPlaceInit, 1, 2, 3, 4, 5};

    ArrayView<int> b1 = a.slice(1, 4);
    CORRADE_COMPARE(b1.size(), 3);
    CORRADE_COMPARE(b1[0], 2);
    CORRADE_COMPARE(b1[1], 3);
    CORRADE_COMPARE(b1[2], 4);

    ArrayView<const int> bc1 = ac.slice(1, 4);
    CORRADE_COMPARE(bc1.size(), 3);
    CORRADE_COMPARE(bc1[0], 2);
    CORRADE_COMPARE(bc1[1], 3);
    CORRADE_COMPARE(bc1[2], 4);

    ArrayView<int> b2 = a.sliceSize(1, 3);
    CORRADE_COMPARE(b2.size(), 3);
    CORRADE_COMPARE(b2[0], 2);
    CORRADE_COMPARE(b2[1], 3);
    CORRADE_COMPARE(b2[2], 4);

    ArrayView<const int> bc2 = ac.sliceSize(1, 3);
    CORRADE_COMPARE(bc2.size(), 3);
    CORRADE_COMPARE(bc2[0], 2);
    CORRADE_COMPARE(bc2[1], 3);
    CORRADE_COMPARE(bc2[2], 4);

    ArrayView<int> c = a.prefix(3);
    CORRADE_COMPARE(c.size(), 3);
    CORRADE_COMPARE(c[0], 1);
    CORRADE_COMPARE(c[1], 2);
    CORRADE_COMPARE(c[2], 3);

    ArrayView<const int> cc = ac.prefix(3);
    CORRADE_COMPARE(cc.size(), 3);
    CORRADE_COMPARE(cc[0], 1);
    CORRADE_COMPARE(cc[1], 2);
    CORRADE_COMPARE(cc[2], 3);

    ArrayView<int> d = a.exceptPrefix(2);
    CORRADE_COMPARE(d.size(), 3);
    CORRADE_COMPARE(d[0], 3);
    CORRADE_COMPARE(d[1], 4);
    CORRADE_COMPARE(d[2], 5);

    ArrayView<const int> dc = ac.exceptPrefix(2);
    CORRADE_COMPARE(dc.size(), 3);
    CORRADE_COMPARE(dc[0], 3);
    CORRADE_COMPARE(dc[1], 4);
    CORRADE_COMPARE(dc[2], 5);

    ArrayView<int> e = a.exceptSuffix(2);
    CORRADE_COMPARE(e.size(), 3);
    CORRADE_COMPARE(e[0], 1);
    CORRADE_COMPARE(e[1], 2);
    CORRADE_COMPARE(e[2], 3);

    ArrayView<const int> ec = ac.exceptSuffix(2);
    CORRADE_COMPARE(ec.size(), 3);
    CORRADE_COMPARE(ec[0], 1);
    CORRADE_COMPARE(ec[1], 2);
    CORRADE_COMPARE(ec[2], 3);
}

void StaticArrayTest::slicePointer() {
    StaticArray<5, int> a{Corrade::InPlaceInit, 1, 2, 3, 4, 5};
    const StaticArray<5, int> ac{Corrade::InPlaceInit, 1, 2, 3, 4, 5};

    ArrayView<int> b1 = a.slice(a + 1, a + 4);
    CORRADE_COMPARE(b1.size(), 3);
    CORRADE_COMPARE(b1[0], 2);
    CORRADE_COMPARE(b1[1], 3);
    CORRADE_COMPARE(b1[2], 4);

    ArrayView<const int> bc1 = ac.slice(ac + 1, ac + 4);
    CORRADE_COMPARE(bc1.size(), 3);
    CORRADE_COMPARE(bc1[0], 2);
    CORRADE_COMPARE(bc1[1], 3);
    CORRADE_COMPARE(bc1[2], 4);

    ArrayView<int> b2 = a.sliceSize(a + 1, 3);
    CORRADE_COMPARE(b2.size(), 3);
    CORRADE_COMPARE(b2[0], 2);
    CORRADE_COMPARE(b2[1], 3);
    CORRADE_COMPARE(b2[2], 4);

    ArrayView<const int> bc2 = ac.sliceSize(ac + 1, 3);
    CORRADE_COMPARE(bc2.size(), 3);
    CORRADE_COMPARE(bc2[0], 2);
    CORRADE_COMPARE(bc2[1], 3);
    CORRADE_COMPARE(bc2[2], 4);

    ArrayView<int> c = a.prefix(a + 3);
    CORRADE_COMPARE(c.size(), 3);
    CORRADE_COMPARE(c[0], 1);
    CORRADE_COMPARE(c[1], 2);
    CORRADE_COMPARE(c[2], 3);

    ArrayView<const int> cc = ac.prefix(ac + 3);
    CORRADE_COMPARE(cc.size(), 3);
    CORRADE_COMPARE(cc[0], 1);
    CORRADE_COMPARE(cc[1], 2);
    CORRADE_COMPARE(cc[2], 3);

    ArrayView<int> d = a.suffix(a + 2);
    CORRADE_COMPARE(d.size(), 3);
    CORRADE_COMPARE(d[0], 3);
    CORRADE_COMPARE(d[1], 4);
    CORRADE_COMPARE(d[2], 5);

    ArrayView<const int> dc = ac.suffix(ac + 2);
    CORRADE_COMPARE(dc.size(), 3);
    CORRADE_COMPARE(dc[0], 3);
    CORRADE_COMPARE(dc[1], 4);
    CORRADE_COMPARE(dc[2], 5);
}

void StaticArrayTest::sliceToStatic() {
    StaticArray<5, int> a{Corrade::InPlaceInit, 1, 2, 3, 4, 5};
    const StaticArray<5, int> ac{Corrade::InPlaceInit, 1, 2, 3, 4, 5};

    StaticArrayView<3, int> b1 = a.slice<3>(1);
    CORRADE_COMPARE(b1[0], 2);
    CORRADE_COMPARE(b1[1], 3);
    CORRADE_COMPARE(b1[2], 4);

    StaticArrayView<3, const int> bc1 = ac.slice<3>(1);
    CORRADE_COMPARE(bc1[0], 2);
    CORRADE_COMPARE(bc1[1], 3);
    CORRADE_COMPARE(bc1[2], 4);

    StaticArrayView<3, int> b2 = a.slice<1, 4>();
    CORRADE_COMPARE(b2[0], 2);
    CORRADE_COMPARE(b2[1], 3);
    CORRADE_COMPARE(b2[2], 4);

    StaticArrayView<3, const int> bc2 = ac.slice<1, 4>();
    CORRADE_COMPARE(bc2[0], 2);
    CORRADE_COMPARE(bc2[1], 3);
    CORRADE_COMPARE(bc2[2], 4);

    StaticArrayView<3, int> b3 = a.sliceSize<1, 3>();
    CORRADE_COMPARE(b3[0], 2);
    CORRADE_COMPARE(b3[1], 3);
    CORRADE_COMPARE(b3[2], 4);

    StaticArrayView<3, const int> bc3 = ac.sliceSize<1, 3>();
    CORRADE_COMPARE(bc3[0], 2);
    CORRADE_COMPARE(bc3[1], 3);
    CORRADE_COMPARE(bc3[2], 4);

    StaticArrayView<3, int> c = a.prefix<3>();
    CORRADE_COMPARE(c[0], 1);
    CORRADE_COMPARE(c[1], 2);
    CORRADE_COMPARE(c[2], 3);

    StaticArrayView<3, const int> cc = ac.prefix<3>();
    CORRADE_COMPARE(cc[0], 1);
    CORRADE_COMPARE(cc[1], 2);
    CORRADE_COMPARE(cc[2], 3);

    StaticArrayView<3, int> d = a.exceptPrefix<2>();
    CORRADE_COMPARE(d[0], 3);
    CORRADE_COMPARE(d[1], 4);
    CORRADE_COMPARE(d[2], 5);

    StaticArrayView<3, const int> cd = ac.exceptPrefix<2>();
    CORRADE_COMPARE(cd[0], 3);
    CORRADE_COMPARE(cd[1], 4);
    CORRADE_COMPARE(cd[2], 5);

    StaticArrayView<3, int> e = a.exceptSuffix<2>();
    CORRADE_COMPARE(e[0], 1);
    CORRADE_COMPARE(e[1], 2);
    CORRADE_COMPARE(e[2], 3);

    StaticArrayView<3, const int> ce = ac.exceptSuffix<2>();
    CORRADE_COMPARE(ce[0], 1);
    CORRADE_COMPARE(ce[1], 2);
    CORRADE_COMPARE(ce[2], 3);

}

void StaticArrayTest::sliceToStaticPointer() {
    StaticArray<5, int> a{Corrade::InPlaceInit, 1, 2, 3, 4, 5};
    const StaticArray<5, int> ac{Corrade::InPlaceInit, 1, 2, 3, 4, 5};

    StaticArrayView<3, int> b = a.slice<3>(a + 1);
    CORRADE_COMPARE(b[0], 2);
    CORRADE_COMPARE(b[1], 3);
    CORRADE_COMPARE(b[2], 4);

    StaticArrayView<3, const int> bc = ac.slice<3>(ac + 1);
    CORRADE_COMPARE(bc[0], 2);
    CORRADE_COMPARE(bc[1], 3);
    CORRADE_COMPARE(bc[2], 4);
}

void StaticArrayTest::sliceZeroNullPointerAmbiguity() {
    StaticArray<5, int> a{Corrade::InPlaceInit, 1, 2, 3, 4, 5};
    const StaticArray<5, int> ac{Corrade::InPlaceInit, 1, 2, 3, 4, 5};

    /* These should all unambigously pick the std::size_t overloads, not the
       T* overloads */

    ArrayView<int> b = a.sliceSize(0, 3);
    CORRADE_COMPARE(b.size(), 3);
    CORRADE_COMPARE(b[0], 1);
    CORRADE_COMPARE(b[1], 2);
    CORRADE_COMPARE(b[2], 3);

    ArrayView<const int> bc = ac.sliceSize(0, 3);
    CORRADE_COMPARE(bc.size(), 3);
    CORRADE_COMPARE(bc[0], 1);
    CORRADE_COMPARE(bc[1], 2);
    CORRADE_COMPARE(bc[2], 3);

    ArrayView<int> c = a.prefix(0);
    CORRADE_COMPARE(c.size(), 0);
    CORRADE_COMPARE(c.data(), static_cast<void*>(a.data()));

    ArrayView<const int> cc = ac.prefix(0);
    CORRADE_COMPARE(cc.size(), 0);
    CORRADE_COMPARE(cc.data(), static_cast<const void*>(ac.data()));

    /** @todo suffix(0), once the non-deprecated suffix(std::size_t size) is a
        thing */

    StaticArrayView<3, int> e = a.slice<3>(0);
    CORRADE_COMPARE(e[0], 1);
    CORRADE_COMPARE(e[1], 2);
    CORRADE_COMPARE(e[2], 3);

    StaticArrayView<3, const int> ec = ac.slice<3>(0);
    CORRADE_COMPARE(ec[0], 1);
    CORRADE_COMPARE(ec[1], 2);
    CORRADE_COMPARE(ec[2], 3);
}

void StaticArrayTest::cast() {
    StaticArray<6, std::uint32_t> a;
    const StaticArray<6, std::uint32_t> ca;
    StaticArray<6, const std::uint32_t> ac;
    const StaticArray<6, const std::uint32_t> cac;

    auto b = arrayCast<std::uint64_t>(a);
    auto bc = arrayCast<const std::uint64_t>(ac);
    auto cb = arrayCast<const std::uint64_t>(ca);
    auto cbc = arrayCast<const std::uint64_t>(cac);

    auto d = arrayCast<std::uint16_t>(a);
    auto dc = arrayCast<const std::uint16_t>(ac);
    auto cd = arrayCast<const std::uint16_t>(ca);
    auto cdc = arrayCast<const std::uint16_t>(cac);

    CORRADE_VERIFY(std::is_same<decltype(b), StaticArrayView<3, std::uint64_t>>::value);
    CORRADE_VERIFY(std::is_same<decltype(bc), StaticArrayView<3, const std::uint64_t>>::value);
    CORRADE_VERIFY(std::is_same<decltype(cb), StaticArrayView<3, const std::uint64_t>>::value);
    CORRADE_VERIFY(std::is_same<decltype(cbc), StaticArrayView<3, const std::uint64_t>>::value);

    CORRADE_VERIFY(std::is_same<decltype(d), StaticArrayView<12,  std::uint16_t>>::value);
    CORRADE_VERIFY(std::is_same<decltype(cd), StaticArrayView<12, const std::uint16_t>>::value);
    CORRADE_VERIFY(std::is_same<decltype(dc), StaticArrayView<12, const std::uint16_t>>::value);
    CORRADE_VERIFY(std::is_same<decltype(cdc), StaticArrayView<12, const std::uint16_t>>::value);

    CORRADE_COMPARE(reinterpret_cast<void*>(b.begin()), reinterpret_cast<void*>(a.begin()));
    CORRADE_COMPARE(reinterpret_cast<const void*>(cb.begin()), reinterpret_cast<const void*>(ca.begin()));
    CORRADE_COMPARE(reinterpret_cast<const void*>(bc.begin()), reinterpret_cast<const void*>(ac.begin()));
    CORRADE_COMPARE(reinterpret_cast<const void*>(cbc.begin()), reinterpret_cast<const void*>(cac.begin()));

    CORRADE_COMPARE(reinterpret_cast<void*>(d.begin()), reinterpret_cast<void*>(a.begin()));
    CORRADE_COMPARE(reinterpret_cast<const void*>(cd.begin()), reinterpret_cast<const void*>(ca.begin()));
    CORRADE_COMPARE(reinterpret_cast<const void*>(dc.begin()), reinterpret_cast<const void*>(ac.begin()));
    CORRADE_COMPARE(reinterpret_cast<const void*>(cdc.begin()), reinterpret_cast<const void*>(cac.begin()));
}

void StaticArrayTest::size() {
    StaticArray<5, int> a;

    CORRADE_COMPARE(arraySize(a), 5);
}

void StaticArrayTest::constructorExplicitInCopyInitialization() {
    /* See constructHelpers.h for details about this compiler-specific issue */
    struct ExplicitDefault {
        explicit ExplicitDefault() {}
    };

    /* The DefaultInit constructor has a special case for
       non-trivially-constructible initialization that's affected by this
       issue as well, be sure to have it picked in this test. This check
       corresponds to the check in the code itself. */
    #ifdef CORRADE_NO_STD_IS_TRIVIALLY_TRAITS
    CORRADE_VERIFY(!Implementation::IsTriviallyConstructibleOnOldGcc<ExplicitDefault>::value);
    #else
    CORRADE_VERIFY(!std::is_trivially_constructible<ExplicitDefault>::value);
    #endif

    struct ContainingExplicitDefaultWithImplicitConstructor {
        ExplicitDefault a;
    };

    /* This alone works */
    ContainingExplicitDefaultWithImplicitConstructor a;
    static_cast<void>(a);

    /* So this should too */
    StaticArray<3, ContainingExplicitDefaultWithImplicitConstructor> b{Corrade::DefaultInit};
    StaticArray<3, ContainingExplicitDefaultWithImplicitConstructor> c{Corrade::ValueInit};
    StaticArray<3, ContainingExplicitDefaultWithImplicitConstructor> d{Corrade::DirectInit};
    CORRADE_COMPARE(b.size(), 3);
    CORRADE_COMPARE(c.size(), 3);
    CORRADE_COMPARE(d.size(), 3);
}

void StaticArrayTest::copyConstructPlainStruct() {
    struct ExtremelyTrivial {
        int a;
        char b;
    };

    /* This needs special handling on GCC 4.8, where T{b} (copy-construction)
       attempts to convert ExtremelyTrivial to int to initialize the first
       argument and fails miserably. */
    StaticArray<3, ExtremelyTrivial> a{Corrade::DirectInit, 3, 'a'};
    CORRADE_COMPARE(a.front().a, 3);

    /* This copy-constructs new values */
    StaticArray<3, ExtremelyTrivial> b{a};
    CORRADE_COMPARE(b.front().a, 3);
}

void StaticArrayTest::moveConstructPlainStruct() {
    /* Can't make MoveOnlyStruct directly non-copyable because then we'd hit
       another GCC 4.8 bug where it can't be constructed using {} anymore. In
       other tests I simply add a (move-only) Array or Pointer member, but here
       I don't want to avoid a needless header dependency. */
    struct MoveOnlyPointer {
        MoveOnlyPointer(std::nullptr_t) {}
        MoveOnlyPointer(const MoveOnlyPointer&) = delete;
        MoveOnlyPointer(MoveOnlyPointer&&) = default;
        MoveOnlyPointer& operator=(const MoveOnlyPointer&) = delete;
        /* Clang complains this function is unused. But removing it may have
           unintended consequences, so don't. */
        CORRADE_UNUSED MoveOnlyPointer& operator=(MoveOnlyPointer&&) = default;

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
    StaticArray<3, MoveOnlyStruct> a{Corrade::DirectInit, 3, 'a', nullptr};
    CORRADE_COMPARE(a.front().a, 3);

    /* This move-constructs new values */
    StaticArray<3, MoveOnlyStruct> b{Utility::move(a)};
    CORRADE_COMPARE(b.front().a, 3);
}

}}}}

CORRADE_TEST_MAIN(Corrade::Containers::Test::StaticArrayTest)
