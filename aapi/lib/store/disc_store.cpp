#include "disc_store.h"

#include <util/folder/filelist.h>
#include <util/random/random.h>
#include <util/string/hex.h>
#include <util/stream/file.h>
#include <util/system/fs.h>

namespace {

TString ToLower(const TString& s) {
    TString ret(s);
    ret.to_lower();
    return ret;
}

}  // namespace

namespace NAapi {
namespace NStore {

TDiscStore::TDiscStore(const TString& path)
    : Path(path)
{
    ui8 b = 0;
    do {
        const TString dir = JoinFsPaths(Path, ToLower(HexEncode(&b, 1)));
        if (!NFs::Exists(dir)) {
            Y_ENSURE(NFs::MakeDirectoryRecursive(dir));
        }
    } while (++b);

    TempDir.Reset(new TTempDir(JoinFsPaths(Path, ToString(RandomNumber<ui64>()))));
}

TString TDiscStore::InnerPath(const TStringBuf &key) const {
    const TString enc = ToLower(HexEncode(key.data(), key.size()));
    return JoinFsPaths(Path, enc.substr(0, 2), enc);
}

TString TDiscStore::TmpPath() const {
    return JoinFsPaths(TempDir->Name(), ToString(RandomNumber<ui64>()));
}

TStatus TDiscStore::Put(const TStringBuf &key, const TString &data) {
    const TString innerPath(InnerPath(key));
    const TString tmpPath(TmpPath());

    try {
        TFile tmpFile(tmpPath, CreateAlways | WrOnly | Seq | Temp);
        TUnbufferedFileOutput output(tmpFile);
        output.Write(data);
        Y_ENSURE(NFs::Rename(tmpPath, innerPath));
    } catch (yexception e) {
        return TStatus::Failed(e.what());
    }

    return TStatus::Ok();
}

TStatus TDiscStore::Get(const TStringBuf &key, TString &data) {
    const TString innerPath(InnerPath(key));

    if (!NFs::Exists(innerPath)) {
        return TStatus::NotFound();
    }

    try {
        TUnbufferedFileInput input(innerPath);
        data.assign(input.ReadAll());
    } catch (yexception e) {
        return TStatus::Failed(e.what());
    }

    return TStatus::Ok();
}

bool TDiscStore::Has(const TStringBuf& key) const {
    return NFs::Exists(InnerPath(key));
}

i64 TDiscStore::Size(const TStringBuf& key) const {
    return GetFileLength(InnerPath(key));
}

TStatus TDiscStore::PutPath(const TString& key, const TString& path) {
    const TString innerPath(InnerPath(key));
    const TString tmpPath(TmpPath());

    try {
        NFs::Copy(path, tmpPath);
        Y_ENSURE(NFs::Rename(tmpPath, innerPath));
    } catch (yexception e) {
        return TStatus::Failed(e.what());
    }

    return TStatus::Ok();
}

TStatus TDiscStore::PutTmpPath(const TStringBuf& key, const TString& tmpPath) {
    const TString innerPath(InnerPath(key));

    try {
        Y_ENSURE(NFs::Rename(tmpPath, innerPath));
    } catch (yexception e) {
        return TStatus::Failed(e.what());
    }

    return TStatus::Ok();
}

}  // namespace NStore
}  // namespace NAapi
