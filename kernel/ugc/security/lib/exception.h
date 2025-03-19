#pragma once

#include <util/generic/bt_exception.h>
#include <util/generic/yexception.h>

namespace NUgc {
    namespace NSecurity {
        // Parent class of all exceptions in the security library.
        class TSecurityException: public yexception {};

        // User data failed a security check. May be an attack. Log and monitor this.
        class TUserException: public TSecurityException {};

        // Incorrect parameters were passed to the library. Fix your code.
        class TApplicationException: public TWithBackTrace<TSecurityException> {};

        // Something failed inside the library. Sorry.
        class TInternalException: public TWithBackTrace<TSecurityException> {};
    } // namespace NSecurity
} // namespace NUgc
