#include "share_object.h"

#include <util/string/hex.h>

namespace NSkyboneD {

TShareObject::TShareObject(TString path, TString link, bool executable)
    : Size_(0)
    , Finished_(false)
    , Path_(std::move(path))
    , HttpLink_(std::move(link))
    , Executable_(executable)
    , Sha1Hasher_(MakeHolder<NOpenSsl::NSha1::TCalcer>())
    , CurrentSha1BlockSize_(0)
{
}

TShareObject& TShareObject::SetPath(TString path) {
    Path_ = std::move(path);
    return *this;
}

TShareObject& TShareObject::SetPublicHttpLink(TString link) {
    Y_ENSURE(link);
    HttpLink_ = std::move(link);
    return *this;
}

TShareObject& TShareObject::SetExecutable(bool executable) {
    Executable_ = executable;
    return *this;
}

TShareObject& TShareObject::SetFingerprint(size_t size, TString md5, TString magicSha1) {
    Y_ENSURE(!Size_ && !Finished_);
    Size_ = size;
    Md5Hash_ = std::move(md5);
    MagicSha1String_ = std::move(magicSha1);
    Finished_ = true;
    return *this;
}

TString TShareObject::GetPath() const {
    return Path_;
}

TString TShareObject::GetPublicHttpLink() const {
    return HttpLink_;
}

bool TShareObject::GetExecutable() const {
    return Executable_;
}

size_t TShareObject::GetSize() const {
    return Size_;
}

TString TShareObject::GetMd5() const {
    if (!Finished_) {
        ythrow yexception() << "file " << Path_ << " is not finished yet";
    }

    return Md5Hash_;
}

TString TShareObject::GetMagicSha1String() const {
    if (!Finished_) {
        ythrow yexception() << "file " << Path_ << " is not finished yet";
    }
    return MagicSha1String_;
}

void TShareObject::DoWrite(const void* buf, size_t len) {
    Y_ENSURE(!Finished_);

    Size_ += len;
    Md5Hasher_.Update(buf, len);

    const char* data = static_cast<const char*>(buf);
    while (len) {
        const size_t count = Min(len, RBTORRENT_BLOCK_SIZE - CurrentSha1BlockSize_);
        Sha1Hasher_->Update(data, count);
        data += count;
        len -= count;
        CurrentSha1BlockSize_ += count;
        if (CurrentSha1BlockSize_ == RBTORRENT_BLOCK_SIZE) {
            auto digest = Sha1Hasher_->Final();
            MagicSha1String_ += TStringBuf{reinterpret_cast<const char*>(digest.data()), digest.size()};
            Sha1Hasher_ = MakeHolder<NOpenSsl::NSha1::TCalcer>();
            CurrentSha1BlockSize_ = 0;
        }
    }
}

void TShareObject::DoFinish() {
    Y_ENSURE(!Finished_);

    if (Size_ == 0 || CurrentSha1BlockSize_ != 0) {
        auto digest = Sha1Hasher_->Final();
        MagicSha1String_ += TStringBuf{reinterpret_cast<const char*>(digest.data()), digest.size()};
    }

    char md5[33];
    Md5Hash_ = TString(Md5Hasher_.End(md5));
    Finished_ = true;
}



}
