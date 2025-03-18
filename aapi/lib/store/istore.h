#pragma once

#include "status.h"

#include <util/generic/strbuf.h>
#include <util/generic/vector.h>
#include <util/memory/blob.h>

namespace NAapi {
namespace NStore {

class IStore {
public:
    virtual ~IStore() {}

    virtual TStatus Put(const TStringBuf& key, const TString& data) = 0;

    virtual TStatus Get(const TStringBuf& key, TString& data) = 0;

//    virtual TStatus BatchPut(const TVector<TStringBuf>& keys, const TVector<TBlob>& data) = 0;
//
//    virtual TStatus BatchPut(const TVector<TString>& keys, const TVector<TBlob>& data) = 0;
//
//    virtual TStatus BatchGet(const TVector<TStringBuf>& keys, TVector<TBlob>& data) = 0;
//
//    virtual TStatus BatchGet(const TVector<TString>& keys, TVector<TBlob>& data) = 0;
};

}  // namespace NStore
}  // namespace NAapi
