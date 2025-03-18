#include "callbacks.h"

namespace NLibgit2 {
    int SkipCertificateCheck(git_cert* /*cert*/, int /*valid*/, const char* /*host*/, void* /*payload*/) {
        return 0;
    }
}
