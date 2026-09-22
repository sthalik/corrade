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

#include "Corrade/Containers/StringView.h"
#include "Corrade/TestSuite/Tester.h"

namespace Corrade { namespace Containers { namespace Test { namespace {

struct StringViewCpp14Test: TestSuite::Tester {
    explicit StringViewCpp14Test();

    void constructPointerConstexpr();
    void constructPointerNullConstexpr();
    void constructPointerFlagsConstexpr();
};

using namespace Literals;

StringViewCpp14Test::StringViewCpp14Test() {
    addTests({&StringViewCpp14Test::constructPointerConstexpr,
              &StringViewCpp14Test::constructPointerNullConstexpr,
              &StringViewCpp14Test::constructPointerFlagsConstexpr});
}

void StringViewCpp14Test::constructPointerConstexpr() {
    constexpr StringView a = "hello";
    constexpr std::size_t size = a.size();
    constexpr StringViewFlags flags = a.flags();
    CORRADE_COMPARE(size, 5);
    CORRADE_COMPARE(flags, StringViewFlag::NullTerminated);
    CORRADE_COMPARE(a, "hello"_s);
}

void StringViewCpp14Test::constructPointerNullConstexpr() {
    constexpr const char* data = nullptr;
    constexpr StringView a = data;
    constexpr const char* aData = a.data();
    constexpr std::size_t size = a.size();
    constexpr StringViewFlags flags = a.flags();
    CORRADE_VERIFY(!aData);
    CORRADE_COMPARE(size, 0);
    CORRADE_COMPARE(flags, StringViewFlag::Global);
}

void StringViewCpp14Test::constructPointerFlagsConstexpr() {
    constexpr StringView a{"hi", StringViewFlag::Global};
    constexpr std::size_t size = a.size();
    constexpr StringViewFlags flags = a.flags();
    CORRADE_COMPARE(size, 2);
    CORRADE_COMPARE(flags, StringViewFlag::Global|StringViewFlag::NullTerminated);
}

}}}}

CORRADE_TEST_MAIN(Corrade::Containers::Test::StringViewCpp14Test)
