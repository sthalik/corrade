#ifndef Corrade_Utility_DebugStream_h
#define Corrade_Utility_DebugStream_h
// todo: add mosra's preferred license boilerplate

#include "Debug.h"
#include <ostream>

namespace Corrade { namespace Utility {

class DebugStream {
    std::ostream* s;

public:
    static inline std::ostream* ostream(Debug::StreamFwd* s) { return s ? reinterpret_cast<std::ostream*>(s) : nullptr; }
    static inline Debug::StreamFwd* fwd(std::ostream* ostream) { return ostream ? reinterpret_cast<Debug::StreamFwd*>(ostream) : nullptr; }

    inline DebugStream(std::ostream* s) : s{s} {}
    inline DebugStream(Debug::StreamFwd* s) : s{ostream(s)} {}
    inline DebugStream(std::nullptr_t) : s{nullptr} {}

    inline operator Debug::StreamFwd*() const noexcept { return fwd(s); }
    inline operator std::ostream*() const noexcept { return s; }

    friend inline bool operator==(const DebugStream& a, const DebugStream& b) noexcept { return a.s == b.s; }
#ifndef CORRADE_TARGET_CXX20
    friend inline operator!=(const DebugStream& a, const DebugStream& b) noexcept { return a.s != b.s; }
#endif
};

}}

#endif
