#pragma once

#include <library/cpp/digest/md5/md5.h>
#include <library/cpp/openssl/crypto/sha.h>

#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/size_literals.h>
#include <util/stream/output.h>

namespace NSkyboneD {

inline constexpr size_t RBTORRENT_BLOCK_SIZE = 4_MB;

class TShareObject : public IOutputStream  {
public:
    TShareObject(TString path, TString link, bool executable);

    TShareObject& SetPath(TString path);
    TShareObject& SetPublicHttpLink(TString link);
    TShareObject& SetExecutable(bool executable);

    // forbids any usage of IOutputStream()
    TShareObject& SetFingerprint(size_t size, TString md5, TString magicSha1);

    TString GetPath() const;
    TString GetPublicHttpLink() const;
    bool GetExecutable() const;
    size_t GetSize() const;

    TString GetMd5() const;
    TString GetMagicSha1String() const;

protected:
    void DoWrite(const void* buf, size_t len) override;
    void DoFinish() override;

private:
    size_t Size_;
    bool Finished_;
    TString Path_;
    TString HttpLink_;
    bool Executable_;

    MD5 Md5Hasher_;
    THolder<NOpenSsl::NSha1::TCalcer> Sha1Hasher_;

    TString Md5Hash_;
    TString MagicSha1String_;
    size_t CurrentSha1BlockSize_;
};
using TShareObjectPtr = TAtomicSharedPtr<TShareObject>;

}
