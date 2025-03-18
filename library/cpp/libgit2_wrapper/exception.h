#pragma once

#include <contrib/libs/libgit2/include/git2.h>

#include <util/generic/yexception.h>

namespace NLibgit2 {

    class TGitException : public yexception {
    public:
        explicit TGitException(int code)
                : TGitException(code, git_error_last()) {
        }

        TGitException(int code, const git_error *error) {
            *this << "code=" << code << ", message=\"" << (error->message ? error->message : "???")
                  << "\" LIBGIT2_ERR_END ";
        }
    };

    class TException : public yexception {
    };

    inline void GitThrowIfError(int error_code, const TStringBuf& message = "") {
        if (error_code == 0) {
            return;
        }

        ythrow TWithBackTrace<TGitException>(error_code) << message;
    }

}
