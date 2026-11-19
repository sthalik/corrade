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

#include "Corrade/Containers/Optional.h"
#include "Corrade/TestSuite/Tester.h"

namespace Corrade { namespace Containers { namespace Test { namespace {

struct OptionalCpp14Test: TestSuite::Tester {
    explicit OptionalCpp14Test();

    void assignReferenceConstexpr();
    void emplaceReferenceConstexpr();
};

OptionalCpp14Test::OptionalCpp14Test() {
    addTests({&OptionalCpp14Test::assignReferenceConstexpr,
              &OptionalCpp14Test::emplaceReferenceConstexpr});
}

constexpr int ValueA = 5;
constexpr int ValueB = 6;

constexpr Optional<const int&> referenceAfterAssignment() {
    Optional<const int&> a = ValueA;
    Optional<const int&> b = ValueB;
    a = b;
    return a;
}

constexpr bool isEmptyAfterReset() {
    Optional<const int&> a = ValueA;
    a = NullOpt;
    return !a;
}

void OptionalCpp14Test::assignReferenceConstexpr() {
    constexpr Optional<const int&> a = referenceAfterAssignment();
    constexpr const int* pointer = a.operator->();
    constexpr bool isEmpty = isEmptyAfterReset();
    CORRADE_VERIFY(pointer == &ValueB);
    CORRADE_VERIFY(isEmpty);
}

constexpr Optional<const int&> referenceAfterEmplace() {
    Optional<const int&> a;
    a.emplace(ValueA);
    const int& b = a.emplace(ValueB);
    return &b == &ValueB ? a : Optional<const int&>{};
}

void OptionalCpp14Test::emplaceReferenceConstexpr() {
    constexpr Optional<const int&> a = referenceAfterEmplace();
    constexpr const int* pointer = a.operator->();
    CORRADE_VERIFY(pointer == &ValueB);
}

}}}}

CORRADE_TEST_MAIN(Corrade::Containers::Test::OptionalCpp14Test)
