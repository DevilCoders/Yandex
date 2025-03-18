#pragma once

#include <contrib/libs/libgit2/include/git2.h>

#include <util/folder/path.h>
#include <util/generic/string.h>

#include <functional>

// Callback types and default callbacks.
namespace NLibgit2 {

    class TCredentials;
    struct TDiffDelta;
    class TTreeEntryRef;

    using TCheckCertCallback = int (*)(git_cert*, int, const char *, void *);
    using TFileCallback = std::function<void(TTreeEntryRef, const TFsPath&)>;
    using TDeltaCallback = std::function<void(TDiffDelta, float)>;
    using TCredentialCallback = std::function<TCredentials(const TString&, const TString&, unsigned int)>;

    int SkipCertificateCheck(git_cert* /*cert*/, int /*valid*/, const char* /*host*/, void* /*payload*/);

} // namespace NLibgit2
