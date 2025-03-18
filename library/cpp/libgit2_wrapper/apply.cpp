#include "apply.h"

#include "diff.h"
#include "exception.h"

#include <contrib/libs/libgit2/include/git2.h>

#include <util/generic/strbuf.h>

namespace NLibgit2 {

    TString ApplyPatch(
        const TString& source,
        const TString& hunksData,
        const TString& path)
    {
        // add dummy information to header otherwise libgit2 fails to parse the patch header
        // refer to patch header parsing transition table in
        // "contrib/libs/libgit2/src/patch_parse.c", line number 367
        const TString patch = ("diff --git a/" + path + " b/" + path + "\nindex 0000000..0000000 100644\n--- a/" + path + "\n+++ b/" + path + "\n") + hunksData;

        TDiff diff(patch);
        git_buf applyBuf = GIT_BUF_INIT_CONST(NULL, 0);
        char* filename = NULL;
        unsigned int mode;
        const auto error = git_apply_to_buf(&applyBuf, &filename, &mode, source.c_str(), source.size(), diff.Get(), NULL);

        const TString result = TString{TStringBuf{applyBuf.ptr, applyBuf.size}};

        if (error || !result) {
            ythrow TException() << TString(git_error_last()->message);
        }
        return result;
    }
}
