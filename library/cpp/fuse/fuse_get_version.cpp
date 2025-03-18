#include "fuse_get_version.h"

#include <util/generic/yexception.h>
#include <util/stream/pipe.h>
#include <util/string/strip.h>

namespace NFuse {
    TString GetFuseVersion() {
#ifndef _asan_enabled_
        try {
            static const TStringBuf VERSION_PREFIX = "fusermount version: ";

            TPipeInput pipe("fusermount --version");
            TString out = Strip(pipe.ReadAll());

            if (out.StartsWith(VERSION_PREFIX)) {
                return out.substr(VERSION_PREFIX.size());
            }

            return TString();
        } catch (const TSystemError& e) {
            return TString();
        }
#else
        return TString();
#endif
    }
}
