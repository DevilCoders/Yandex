#include "random.h"

#include <util/system/file.h>
#include <util/generic/yexception.h>

namespace NUgc {
    namespace NSecurity {
        namespace NInternal {
            size_t RandomBytes(char* buf, size_t num) {
#ifndef __unix__
                ythrow yexception() << "RandomBytes not implemented for non unix systems.";
#else
                static TFile urand("/dev/urandom", EOpenModeFlag::OpenExisting | EOpenModeFlag::RdOnly);
                return urand.Read(buf, num);
#endif
            }
        } // namespace NInternal
    } // namespace NSecurity
} // namespace NUgc
