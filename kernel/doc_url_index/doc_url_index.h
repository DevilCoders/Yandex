#pragma once

/// author@ vvp@ Victor Ploshikhin
/// created: Oct 25, 2011 6:21:04 PM
/// see: BUKI-1289

#include "interface.h"

#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/yexception.h>

class TDocOmniWadIndex;

// Reads index.docurl
class TDocUrlIndexReader {
private:
    THolder<class TDocUrlIndexReaderImpl> Impl;

protected:
    TDocUrlIndexReader();

public:
    TDocUrlIndexReader(const TString& fileName);
    virtual ~TDocUrlIndexReader();

    virtual TStringBuf Get(const ui32 docId) const /*throw(yexception)*/;
    virtual size_t Size() const;
};

class TDocUrlIndexManager: public IDocUrlIndexManager {
public:
    TDocUrlIndexManager(const TString& oldIndexPath, const TDocOmniWadIndex* newReader = nullptr);
    TStringBuf Get(const ui32 docId, TOmniUrlAccessor* accessor) const override;
    size_t Size() const override;
    ~TDocUrlIndexManager() override;

private:
    THolder<TDocUrlIndexReader> OldReader;
    const TDocOmniWadIndex* NewReader;
};
