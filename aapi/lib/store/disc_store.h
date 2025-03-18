#pragma once

#include "istore.h"

#include <util/folder/tempdir.h>

namespace NAapi {
namespace NStore {

class TDiscStore: public IStore {
public:
    TDiscStore(const TString& path);
    TStatus Put(const TStringBuf& key, const TString& data) override;
    TStatus Get(const TStringBuf& key, TString& data) override;
    bool Has(const TStringBuf& key) const;

    TString InnerPath(const TStringBuf& key) const;
    TString TmpPath() const;
    i64 Size(const TStringBuf& key) const;
    TStatus PutPath(const TString& key, const TString& path);
    TStatus PutTmpPath(const TStringBuf& key, const TString& tmpPath);

private:
    TString Path;
    THolder<TTempDir> TempDir;
};

}  // namespace NStore
}  // namespace NAapi
