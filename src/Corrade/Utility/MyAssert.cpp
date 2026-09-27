#include "MyAssert.h"
#include "Corrade/Utility/Debug.h"
#include <cstdio>
#include <cstdlib>
#include <new>

namespace Corrade { namespace Utility { namespace Implementation { namespace MyAssert {

namespace {

// A struct because alignas must precede thread_local.
struct error_slot
{
    alignas(Utility::Error) unsigned char storage[sizeof(Utility::Error)];
    Utility::Error* active;
};

#ifdef CORRADE_BUILD_MULTITHREADED
CORRADE_THREAD_LOCAL
#endif
error_slot slot;

} // namespace

void emit_abort() noexcept
{
    std::fflush(stdout);
    std::fflush(stderr);
    std::abort();
}

void emit_assert_fail(const char* message) noexcept
{
    std::fflush(stdout);
    Utility::Error{Utility::Error::defaultOutput()} << message;
    emit_abort();
}

Utility::Error& emit_message_begin() noexcept
{
    std::fflush(stdout);
    slot.active = new(slot.storage) Utility::Error{Utility::Error::defaultOutput()};
    return *slot.active;
}

void emit_message_end() noexcept
{
    slot.active->~Error();
    emit_abort();
}

}}}} // namespace Corrade::Utility::Implementation::MyAssert
