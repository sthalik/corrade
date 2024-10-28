/*
    This file is part of Corrade.

    Copyright © 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016,
                2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024
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

#include "Debug.h"
#include "DebugStream.h"

#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <sstream>

/* For isatty() on Unix-like systems */
#ifdef CORRADE_TARGET_UNIX
#include <unistd.h>

/* Node.js alternative to isatty() on Emscripten */
#elif defined(CORRADE_TARGET_EMSCRIPTEN)
#include <emscripten.h>

#elif defined(CORRADE_TARGET_WINDOWS)
#define WIN32_LEAN_AND_MEAN 1
#define VC_EXTRALEAN

/* For isatty() on Windows. Needed by both ANSI and non-ANSI color printing. */
#include <io.h>

/* WINAPI-based colored output on Windows */
#ifndef CORRADE_UTILITY_USE_ANSI_COLORS
#include <windows.h>
#include <wincon.h>
#endif
#endif

#include "Corrade/Containers/EnumSet.hpp"
#include "Corrade/Containers/String.h"
#include "Corrade/Containers/StringView.h"
#include "Corrade/Utility/DebugStl.h"
#include "Corrade/Utility/Macros.h" /* CORRADE_THREAD_LOCAL */

#if defined(CORRADE_TARGET_WINDOWS) && defined(CORRADE_BUILD_STATIC_UNIQUE_GLOBALS) && !defined(CORRADE_TARGET_WINDOWS_RT)
#include "Corrade/Utility/Implementation/WindowsWeakSymbol.h"
#endif

#ifdef CORRADE_SOURCE_LOCATION_BUILTINS_SUPPORTED
#include "Corrade/Utility/Assert.h"
#endif

#ifdef CORRADE_TARGET_EMSCRIPTEN
/* Implemented in Utility.js.in */
extern "C" {
    bool corradeUtilityIsTty(int output);
}
#endif

namespace Corrade { namespace Utility {

namespace {

template<class T> inline void toStream(DebugStream s, const T& value) {
    *DebugStream::ostream(s) << value;
}

template<> inline void toStream(DebugStream s, const Containers::StringView& value) {
    DebugStream::ostream(s)->write(value.data(), value.size());
}

template<> inline void toStream(DebugStream s, const Containers::MutableStringView& value) {
    DebugStream::ostream(s)->write(value.data(), value.size());
}

template<> inline void toStream(DebugStream s, const Containers::String& value) {
    DebugStream::ostream(s)->write(value.data(), value.size());
}

template<> inline void toStream<Implementation::DebugOstreamFallback>(DebugStream s, const Implementation::DebugOstreamFallback& value) {
    value.apply(*DebugStream::ostream(s));
}

#if defined(CORRADE_TARGET_WINDOWS) && !defined(CORRADE_UTILITY_USE_ANSI_COLORS)
HANDLE streamOutputHandle(const std::ostream* s) {
    /* The isatty() is there to detect if the output is redirected to a file.
       If it would be, GetStdHandle() returns a valid handle, but subsequent
       calls to GetConsoleScreenBufferInfo() or SetConsoleTextAttribute() would
       fail because "The handle must have the GENERIC_READ access right.", yes,
       a READ access for an OUTPUT handle, a very helpful documentation and
       basically no possibility of being able to find any further info on this
       except for contemplating my life decisions while staring into a blank
       white door while taking a long dump on a toilet.

       I wouldn't care or bother (and I didn't for the past six or so years
       since the original implementation of this was gifted to me), but having
       an error generated by a debug output printer is somewhat problematic
       when the thing being printed is the value of GetLastError(), such as in
       Implementation::printWindowsErrorString(). Yes, so you always get
       "error 6 (The handle is invalid.)" if CORRADE_UTILITY_USE_ANSI_COLORS
       is not set.

       After having confirmed a suspicion that "GENERIC_READ access right"
       means "not being redirected", the next step was to find a Windows API
       that tells me such a fact. Apparently, the most direct Windows-native™
       way to check this is by calling GetFileInformationByHandle() on the
       handle and ... seeing if it produces an error. Amazing, wonderful. Back
       to the start. Fortunately, Windows is Still A Bit Of Unix At Heart, so
       I just ask isatty(), like I do elsewhere.

       Actually, no, because then I would have to
        #pragma warning(disable: 4996)
       and also
        #pragma clang diagnostic ignored "-Wdeprecated-declarations"
       for clang-cl like I do below for the ANSI build and that's just too much
       already. So I "use the ISO C and C++ conformant name: _isatty" As The
       Computer God Demands. */
    return s == &std::cout && _isatty(1) ? GetStdHandle(STD_OUTPUT_HANDLE) :
           s == &std::cerr && _isatty(2) ? GetStdHandle(STD_ERROR_HANDLE) :
           INVALID_HANDLE_VALUE;
}
#endif

}

#if !defined(CORRADE_BUILD_STATIC_UNIQUE_GLOBALS) || defined(CORRADE_TARGET_WINDOWS)
/* (Of course) can't be in an unnamed namespace in order to export it below
   (except for Windows, where we do extern "C" so this doesn't matter, but we
   don't want to expose the DebugGlobals symbols if not needed) */
namespace {
#endif

struct DebugGlobals {
    Debug::StreamFwd *output, *warningOutput, *errorOutput;
    #if !defined(CORRADE_TARGET_WINDOWS) ||defined(CORRADE_UTILITY_USE_ANSI_COLORS)
    Debug::Color color;
    bool colorBold, colorInverted;
    #endif
};

#ifdef CORRADE_BUILD_MULTITHREADED
CORRADE_THREAD_LOCAL
#endif
#if defined(CORRADE_BUILD_STATIC_UNIQUE_GLOBALS) && !defined(CORRADE_TARGET_WINDOWS)
/* On static builds that get linked to multiple shared libraries and then used
   in a single app we want to ensure there's just one global symbol. On Linux
   it's apparently enough to just export, macOS needs the weak attribute.
   Windows handled differently below. */
CORRADE_VISIBILITY_EXPORT
    #ifdef CORRADE_TARGET_GCC
    __attribute__((weak))
    #else
    /* uh oh? the test will fail, probably */
    #endif
#endif
DebugGlobals debugGlobals{
    #if defined(CORRADE_TARGET_MINGW) && defined(CORRADE_TARGET_CLANG)
    /* Referencing the globals directly makes MinGW Clang segfault for some reason */
    Debug::defaultOutput(), Warning::defaultOutput(), Error::defaultOutput(),
    #else
    &std::cout, &std::cerr, &std::cerr,
    #endif
    #if !defined(CORRADE_TARGET_WINDOWS) ||defined(CORRADE_UTILITY_USE_ANSI_COLORS)
    Debug::Color::Default, false, false
    #endif
};

#if !defined(CORRADE_BUILD_STATIC_UNIQUE_GLOBALS) || defined(CORRADE_TARGET_WINDOWS)
}
#endif

/* Windows can't have a symbol both thread-local and exported, moreover there
   isn't any concept of weak symbols. Exporting thread-local symbols can be
   worked around by exporting a function that then returns a reference to a
   non-exported thread-local symbol; and finally GetProcAddress() on
   GetModuleHandle(nullptr) "emulates" the weak linking as it's guaranteed to
   pick up the same symbol of the final exe independently of the DLL it was
   called from. To avoid #ifdef hell in code below, the debugGlobals are
   redefined to return a value from this uniqueness-ensuring function. */
#if defined(CORRADE_TARGET_WINDOWS) && defined(CORRADE_BUILD_STATIC_UNIQUE_GLOBALS) && !defined(CORRADE_TARGET_WINDOWS_RT)
/* Clang-CL complains that the function has a return type incompatible with C.
   I don't care, I only need an unmangled name to look up later at runtime. */
#ifdef CORRADE_TARGET_CLANG_CL
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-type-c-linkage"
#endif
extern "C" CORRADE_VISIBILITY_EXPORT DebugGlobals& corradeUtilityUniqueDebugGlobals();
extern "C" CORRADE_VISIBILITY_EXPORT DebugGlobals& corradeUtilityUniqueDebugGlobals() {
    return debugGlobals;
}
#ifdef CORRADE_TARGET_CLANG_CL
#pragma clang diagnostic pop
#endif

namespace {

DebugGlobals& windowsDebugGlobals() {
    /* A function-local static to ensure it's only initialized once without any
       race conditions among threads */
    static DebugGlobals&(*const uniqueGlobals)() = reinterpret_cast<DebugGlobals&(*)()>(Implementation::windowsWeakSymbol("corradeUtilityUniqueDebugGlobals", reinterpret_cast<void*>(&corradeUtilityUniqueDebugGlobals)));
    return uniqueGlobals();
}

}

#define debugGlobals windowsDebugGlobals()
#endif

template<Debug::Color c, bool bold> Debug::Modifier Debug::colorInternal() {
    return [](Debug& debug) {
        if(!debug._output || (debug._flags & InternalFlag::DisableColors)) return;

        debug._flags |= InternalFlag::ColorWritten|InternalFlag::ValueWritten;
        #if defined(CORRADE_TARGET_WINDOWS) && !defined(CORRADE_UTILITY_USE_ANSI_COLORS)
        HANDLE h = streamOutputHandle(debug._output);
        if(h != INVALID_HANDLE_VALUE) SetConsoleTextAttribute(h,
            (debug._previousColorAttributes & ~(FOREGROUND_BLUE|FOREGROUND_GREEN|FOREGROUND_RED|FOREGROUND_INTENSITY)) |
            char(c) |
            (bold ? FOREGROUND_INTENSITY : 0));
        #else
        debugGlobals.color = c;
        debugGlobals.colorBold = bold;
        debugGlobals.colorInverted = false;
        if(bold) {
            /* Adds an extra reset to also undo the inverse, if was set
               before */
            constexpr const char code[]{'\033', '[', '0', ';', '1' , ';', '3', '0' + char(c), 'm', '\0'};
            *DebugStream::ostream(debug._output) << code;
        } else {
            /* This resets both bold and inverse */
            constexpr const char code[]{'\033', '[', '0', ';', '3', '0' + char(c), 'm', '\0'};
            *DebugStream::ostream(debug._output) << code;
        }
        #endif
    };
}

#if !defined(CORRADE_TARGET_WINDOWS) || defined(CORRADE_UTILITY_USE_ANSI_COLORS)
template<Debug::Color c> Debug::Modifier Debug::invertedColorInternal() {
    return [](Debug& debug) {
        if(!debug._output || (debug._flags & InternalFlag::DisableColors)) return;

        debug._flags |= InternalFlag::ColorWritten|InternalFlag::ValueWritten;
        debugGlobals.color = c;
        debugGlobals.colorBold = false;
        debugGlobals.colorInverted = true;
        /* Adds an extra reset to also undo the bold, if was set before */
        constexpr const char code[] = { '\033', '[', '0', ';', '7', ';', '3', '0' + char(c), 'm', '\0' };
        *DebugStream::ostream(debug._output) << code;
    };
}
#endif

inline void Debug::resetColorInternal() {
    if(!_output || !(_flags & InternalFlag::ColorWritten)) return;

    _flags &= ~InternalFlag::ColorWritten;
    _flags |= InternalFlag::ValueWritten;
    #if defined(CORRADE_TARGET_WINDOWS) && !defined(CORRADE_UTILITY_USE_ANSI_COLORS)
    HANDLE h = streamOutputHandle(_output);
    if(h != INVALID_HANDLE_VALUE)
        SetConsoleTextAttribute(h, _previousColorAttributes);
    #else
    if(_previousColorBold || _previousColorInverted) {
        /* Only one of the two should be set by our code */
        CORRADE_INTERNAL_ASSERT(!_previousColorBold || !_previousColorInverted);
        const char code[]{'\033', '[', '0', ';', _previousColorBold ? '1' : '7', ';', '3', char('0' + char(_previousColor)), 'm', '\0'};
        *DebugStream::ostream(_output) << code;
    } else if(_previousColor != Color::Default) {
        const char code[]{'\033', '[', '0', ';', '3', char('0' + char(_previousColor)), 'm', '\0'};
        *DebugStream::ostream(_output) << code;
    } else *DebugStream::ostream(_output) << "\033[0m";

    debugGlobals.color = _previousColor;
    debugGlobals.colorBold = _previousColorBold;
    debugGlobals.colorInverted = _previousColorInverted;
    #endif
}

auto Debug::color(Color color) -> Modifier {
    /* Crazy but working solution to work around the need for capturing lambda
       which disallows converting it to function pointer */
    switch(color) {
        #define _c(color) case Color::color: return colorInternal<Color::color, false>();
        _c(Black)
        _c(Red)
        _c(Green)
        _c(Yellow)
        _c(Blue)
        _c(Magenta)
        _c(Cyan)
        _c(White)
        #if !defined(CORRADE_TARGET_WINDOWS) || defined(CORRADE_UTILITY_USE_ANSI_COLORS)
        _c(Default)
        #endif
        #undef _c
    }

    CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
}

auto Debug::boldColor(Color color) -> Modifier {
    /* Crazy but working solution to work around the need for capturing lambda
       which disallows converting it to function pointer */
    switch(color) {
        #define _c(color) case Color::color: return colorInternal<Color::color, true>();
        _c(Black)
        _c(Red)
        _c(Green)
        _c(Yellow)
        _c(Blue)
        _c(Magenta)
        _c(Cyan)
        _c(White)
        #if !defined(CORRADE_TARGET_WINDOWS) || defined(CORRADE_UTILITY_USE_ANSI_COLORS)
        _c(Default)
        #endif
        #undef _c
    }

    CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
}

#if !defined(CORRADE_TARGET_WINDOWS) || defined(CORRADE_UTILITY_USE_ANSI_COLORS)
auto Debug::invertedColor(Color color) -> Modifier {
    /* Crazy but working solution to work around the need for capturing lambda
       which disallows converting it to function pointer */
    switch(color) {
        #define _c(color) case Color::color: return invertedColorInternal<Color::color>();
        _c(Black)
        _c(Red)
        _c(Green)
        _c(Yellow)
        _c(Blue)
        _c(Magenta)
        _c(Cyan)
        _c(White)
        _c(Default)
        #undef _c
    }

    CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
}
#endif

void Debug::resetColor(Debug& debug) {
    debug.resetColorInternal();
}

namespace { enum: unsigned short { PublicFlagMask = 0x00ff }; }

auto Debug::flags() const -> Flags {
    return Flag(static_cast<unsigned short>(_flags) & PublicFlagMask);
}

void Debug::setFlags(Flags flags) {
    _flags = InternalFlag(static_cast<unsigned short>(flags)) |
        InternalFlag(static_cast<unsigned short>(_flags) & ~PublicFlagMask);
}

auto Debug::immediateFlags() const -> Flags {
    return Flag(static_cast<unsigned short>(_immediateFlags) & PublicFlagMask) |
        Flag(static_cast<unsigned short>(_flags) & PublicFlagMask);
}

void Debug::setImmediateFlags(Flags flags) {
    /* unlike _flags, _immediateFlags doesn't contain any internal flags so
       no need to preserve these */
    _immediateFlags = InternalFlag(static_cast<unsigned short>(flags));
}

DebugStream Debug::defaultOutput() { return DebugStream::fwd(&std::cout); }
DebugStream Warning::defaultOutput() { return DebugStream::fwd(&std::cerr); }
DebugStream Error::defaultOutput() { return DebugStream::fwd(&std::cerr); }

DebugStream Debug::output() { return debugGlobals.output; }
DebugStream Warning::output() { return debugGlobals.warningOutput; }
DebugStream Error::output() { return debugGlobals.errorOutput; }

bool Debug::isTty(DebugStream output) {
    /* On Windows with WINAPI colors check the stream output handle */
    #if defined(CORRADE_TARGET_WINDOWS) && !defined(CORRADE_UTILITY_USE_ANSI_COLORS)
    return streamOutputHandle(output) != INVALID_HANDLE_VALUE;

    /* We can autodetect via isatty() on Unix-like systems and Windows with
       ANSI colors enabled */
    #elif (defined(CORRADE_TARGET_WINDOWS) && defined(CORRADE_UTILITY_USE_ANSI_COLORS)) || defined(CORRADE_TARGET_UNIX)
    return
        /* Windows RT projects have C4996 treated as error by default. WHY.
           Also, clang-cl doesn't understand warning IDs yet, so using its own
           warning suppression right now. */
        #ifdef CORRADE_TARGET_CLANG_CL
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wdeprecated-declarations"
        #elif defined(CORRADE_TARGET_MSVC)
        #pragma warning(push)
        #pragma warning(disable: 4996)
        #endif
        ((DebugStream::ostream(output) == &std::cout && isatty(1)) ||
         (DebugStream::ostream(output) == &std::cerr && isatty(2)))
        #ifdef CORRADE_TARGET_CLANG_CL
        #pragma clang diagnostic pop
        #elif defined(CORRADE_TARGET_MSVC)
        #pragma warning(pop)
        #endif
        #ifdef CORRADE_TARGET_APPLE
        /* Xcode's console reports that it is a TTY, but it doesn't support
           colors. Originally this was testing for XPC_SERVICE_NAME being
           defined because that's always defined when running inside Xcode, but
           nowadays that's often defined also outside of Xcode, so it's
           useless. According to https://stackoverflow.com/a/39292112, we can
           reliably check for TERM -- if it's null, we're inside Xcode. */
        && std::getenv("TERM")
        #endif
        ;

    /* Emscripten isatty() is broken since forever, so we have to call into
       Node.js: https://github.com/kripken/emscripten/issues/4920 */
    #elif defined(CORRADE_TARGET_EMSCRIPTEN)
    int out = 0;
    if(output == &std::cout)
        out = 1;
    else if(output == &std::cerr)
        out = 2;
    return corradeUtilityIsTty(out);

    /* Otherwise can't be autodetected, thus disable colkors by default */
    #else
    return false;
    #endif
}

bool Debug::isTty() { return isTty(debugGlobals.output); }
bool Warning::isTty() { return Debug::isTty(debugGlobals.warningOutput); }
bool Error::isTty() { return Debug::isTty(debugGlobals.errorOutput); }

Debug::Debug(DebugStream const output, const Flags flags): _flags{InternalFlag(static_cast<unsigned short>(flags))}, _immediateFlags{InternalFlag::NoSpace} {
    /* Save previous global output and replace it with current one */
    _previousGlobalOutput = debugGlobals.output;
    debugGlobals.output = _output = output;

    /* Save previous global color */
    #if defined(CORRADE_TARGET_WINDOWS) && !defined(CORRADE_UTILITY_USE_ANSI_COLORS)
    HANDLE h = streamOutputHandle(_output);
    if(h != INVALID_HANDLE_VALUE) {
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(h, &csbi);
        _previousColorAttributes = csbi.wAttributes;
    }
    #else
    _previousColor = debugGlobals.color;
    _previousColorBold = debugGlobals.colorBold;
    _previousColorInverted = debugGlobals.colorInverted;
    #endif
}

#if !defined(DOXYGEN_GENERATING_OUTPUT) && defined(CORRADE_SOURCE_LOCATION_BUILTINS_SUPPORTED)
namespace Implementation {
DebugSourceLocation::DebugSourceLocation(Debug&& debug, const char* file, int line): debug{&debug} {
    debug._sourceLocationFile = file;
    debug._sourceLocationLine = line;
}
}
#endif

Warning::Warning(DebugStream const output, const Flags flags): Debug{flags} {
    /* Save previous global output and replace it with current one */
    _previousGlobalWarningOutput = debugGlobals.warningOutput;
    debugGlobals.warningOutput = _output = output;
}

Error::Error(DebugStream const output, const Flags flags): Debug{flags} {
    /* Save previous global output and replace it with current one */
    _previousGlobalErrorOutput = debugGlobals.errorOutput;
    debugGlobals.errorOutput = _output = output;
}

Debug::Debug(const Flags flags): Debug{debugGlobals.output, flags} {}
Warning::Warning(const Flags flags): Warning{debugGlobals.warningOutput, flags} {}
Error::Error(const Flags flags): Error{debugGlobals.errorOutput, flags} {}

void Debug::cleanupOnDestruction() {
    #ifdef CORRADE_SOURCE_LOCATION_BUILTINS_SUPPORTED
    /* Print source location if not printed yet -- this means saying a
       !Debug{}; will print just that, while Debug{}; is a no-op */
    if(_output && _sourceLocationFile) {
        CORRADE_INTERNAL_ASSERT(_immediateFlags & InternalFlag::NoSpace);
        *DebugStream::ostream(_output) << _sourceLocationFile << ":" << _sourceLocationLine;
        _flags |= InternalFlag::ValueWritten;
    }
    #endif

    /* Reset output color */
    resetColorInternal();

    /* Newline at the end */
    if(_output && (_flags & InternalFlag::ValueWritten) && !(_flags & InternalFlag::NoNewlineAtTheEnd))
        *DebugStream::ostream(_output) << "\n";

    /* Reset previous global output */
    debugGlobals.output = _previousGlobalOutput;
}

Debug::~Debug() {
    cleanupOnDestruction();
}

Warning::~Warning() {
    debugGlobals.warningOutput = _previousGlobalWarningOutput;
}

void Error::cleanupOnDestruction() {
    debugGlobals.errorOutput = _previousGlobalErrorOutput;
}

Error::~Error() {
    cleanupOnDestruction();
}

Fatal::Fatal(DebugStream output, int exitCode, Flags flags): Error{output, flags}, _exitCode{exitCode} {}
Fatal::Fatal(DebugStream output, Flags flags): Fatal{output, 1, flags} {}

/* MSVC in a Release build complains that "destructor never returns, potential
   memory leak". Well, yes, since this is a [[noreturn]] function. */
#if defined(CORRADE_TARGET_MSVC) && !defined(CORRADE_TARGET_CLANG_CL)
#pragma warning(push)
#pragma warning(disable: 4722)
#endif
Fatal::~Fatal() {
    /* Manually call cleanup of Error and Debug superclasses because their
       destructor will never be called */
    Error::cleanupOnDestruction();
    Debug::cleanupOnDestruction();

    std::exit(_exitCode);
}
#if defined(CORRADE_TARGET_MSVC) && !defined(CORRADE_TARGET_CLANG_CL)
#pragma warning(pop)
#endif

template<class T> Debug& Debug::print(const T& value) {
    if(!_output) return *this;

    #ifdef CORRADE_SOURCE_LOCATION_BUILTINS_SUPPORTED
    /* Print source location, if not printed yet */
    if(_sourceLocationFile) {
        CORRADE_INTERNAL_ASSERT(_immediateFlags & InternalFlag::NoSpace);
        *DebugStream::ostream(_output) << _sourceLocationFile << ":" << _sourceLocationLine << ": ";
        _sourceLocationFile = nullptr;
    }
    #endif

    /* Separate values with spaces if enabled */
    if(!((_immediateFlags|_flags) & InternalFlag::NoSpace))
        *DebugStream::ostream(_output) << ' ';
    /* Print the next value as hexadecimal if enabled */
    /** @todo this does strange crap for negative values (printing them as
        unsigned), revisit once iostreams are not used anymore */
    if(((_immediateFlags|_flags) & InternalFlag::Hex) && std::is_integral<T>::value)
        *DebugStream::ostream(_output) << "0x" << std::hex;

    toStream(_output, value);

    /* Reset the hexadecimal printing back if it was enabled */
    if(((_immediateFlags|_flags) & InternalFlag::Hex) && std::is_integral<T>::value)
        *DebugStream::ostream(_output) << std::dec;

    /* Reset all internal flags after */
    _immediateFlags = {};

    _flags |= InternalFlag::ValueWritten;
    return *this;
}

Debug& Debug::operator<<(const void* const value) {
    return *this << hex << reinterpret_cast<std::uintptr_t>(value);
}

Debug& Debug::operator<<(const char* value) { return print(value); }
Debug& Debug::operator<<(Containers::StringView value) { return print(value); }
Debug& Debug::operator<<(Containers::MutableStringView value) { return print(value); }
Debug& Debug::operator<<(const Containers::String& value) { return print(value); }

Debug& Debug::operator<<(bool value) { return print(value ? "true" : "false"); }
Debug& Debug::operator<<(int value) { return print(value); }

Debug& Debug::operator<<(char value) { return print(int(value)); }

Debug& Debug::operator<<(unsigned char value) {
    /* Convert to int to avoid infinite recursion to operator<<(unsigned char) */
    const int v = value;

    /* Print the value as a shade of gray */
    if(immediateFlags() & Flag::Color) {
        const char* shade;
        if(value < 51)       shade = "  ";
        else if(value < 102) shade = "░░";
        else if(value < 153) shade = "▒▒";
        else if(value < 204) shade = "▓▓";
        else                 shade = "██";

        /* If ANSI colors are disabled, use just the shade */
        if(immediateFlags() & Flag::DisableColors)
            return print(shade);
        else {
            print("\033[38;2;");

            /* Disable space between values for everything after the initial
               value */
            const Flags previousFlags = flags();
            setFlags(previousFlags|Flag::NoSpace);

            /* Set both background and foreground, reset back after */
            *this << v << ";" << v << ";" << v << "m\033[48;2;"
                << v << ";" << v << ";" << v << "m" << shade << "\033[0m";

            /* Reset original flags */
            setFlags(previousFlags);
            return *this;
        }

    /* Otherwise print its numeric value */
    } else return print(v);
}

Debug& Debug::operator<<(long value) { return print(value); }
Debug& Debug::operator<<(long long value) { return print(value); }
Debug& Debug::operator<<(unsigned value) { return print(value); }
Debug& Debug::operator<<(unsigned long value) { return print(value); }
Debug& Debug::operator<<(unsigned long long value) { return print(value); }

Debug& Debug::operator<<(float value) {
    if(!_output) return *this;
    *DebugStream::ostream(_output) << std::setprecision(Implementation::FloatPrecision<float>::Digits);
    return print(value);
}
Debug& Debug::operator<<(double value) {
    if(!_output) return *this;
    *DebugStream::ostream(_output) << std::setprecision(Implementation::FloatPrecision<double>::Digits);
    return print(value);
}
Debug& Debug::operator<<(long double value) {
    if(!_output) return *this;
    *DebugStream::ostream(_output) << std::setprecision(Implementation::FloatPrecision<long double>::Digits);
    return print(value);
}

Debug& Debug::operator<<(char32_t value) {
    std::ostringstream o;
    o << "U+" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << std::uint32_t(value);
    return print(o.str());
}

Debug& Debug::operator<<(const char32_t* value) {
    return *this << std::u32string(value);
}

Debug& Debug::operator<<(std::nullptr_t) {
    return print("nullptr");
}

#ifndef DOXYGEN_GENERATING_OUTPUT
/* Doxygen can't match this to the declaration, eh. */
Debug& operator<<(Debug& debug, Debug::Color value) {
    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case Debug::Color::value: return debug << "Utility::Debug::Color::" #value;
        _c(Black)
        _c(Red)
        _c(Green)
        _c(Yellow)
        _c(Blue)
        _c(Magenta)
        _c(Cyan)
        _c(White)
        #if !defined(CORRADE_TARGET_WINDOWS) || defined(CORRADE_UTILITY_USE_ANSI_COLORS)
        _c(Default) /* Alias to White on Windows */
        #endif
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "Utility::Debug::Color(" << Debug::nospace << Debug::hex << static_cast<unsigned char>(char(value)) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, Debug::Flag value) {
    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(value) case Debug::Flag::value: return debug << "Utility::Debug::Flag::" #value;
        _c(NoNewlineAtTheEnd)
        _c(DisableColors)
        _c(NoSpace)
        _c(Packed)
        _c(Color)
        /* Space reserved for Bin and Oct */
        _c(Hex)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << "Utility::Debug::Flag(" << Debug::nospace << Debug::hex << static_cast<unsigned short>(value) << Debug::nospace << ")";
}

Debug& operator<<(Debug& debug, Debug::Flags value) {
    return Containers::enumSetDebugOutput(debug, value, "Utility::Debug::Flags{}", {
        Debug::Flag::NoNewlineAtTheEnd,
        Debug::Flag::DisableColors,
        Debug::Flag::NoSpace,
        Debug::Flag::Packed,
        Debug::Flag::Color,
        /* Space reserved for Bin and Oct */
        Debug::Flag::Hex});
}
#endif

/* For some reason Doxygen can't match these with the declaration in DebugStl.h */
#ifndef DOXYGEN_GENERATING_OUTPUT
/** @todo when we get rid of iostreams, make this inline in DebugStl.h so we
    don't bloat our binaries with STL symbols */
Debug& Implementation::debugPrintStlString(Debug& debug, const std::string& value) {
    return debug.print(value);
}

Debug& operator<<(Debug& debug, Implementation::DebugOstreamFallback&& value) {
    return debug.print(value);
}
#endif

}}
