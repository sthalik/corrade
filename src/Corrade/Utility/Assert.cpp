#include "Assert.h"
#include "DebugStream.h"

namespace Corrade { namespace Utility {

namespace Implementation {

bool isDefaultErrorOutput() {
    return Error::defaultOutput() == Error::output();
}

Error errorOutputForAssert() {
    return Corrade::Utility::Error{Corrade::Utility::Error::defaultOutput()};
}

}

}}
