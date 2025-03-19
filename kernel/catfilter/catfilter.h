#pragma once

#include <library/cpp/deprecated/mapped_file/mapped_file.h>

#include <util/generic/map.h>
#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/system/defaults.h>
#include <util/system/filemap.h>

typedef ui32 TCat;
typedef ui32 TCatValue;
typedef ui32 TCatAttr;
typedef TVector<TCat> TCats;
typedef TVector<TCatValue> TCatValues;
typedef TVector<TCatAttr> TCatAttrs;
typedef TMap<TCat, TCatValues> TCatToCatValues;
typedef TMap<TCat, ui32> TCatToNestingDepth;
typedef TAtomicSharedPtr<const TCatAttrs> TCatAttrsPtr;

TCatAttrsPtr CatFilterFind0(const char *relurl);
TCatAttrsPtr CatFilterFind(const char *url);
TCatAttrsPtr CatFilterFind2(const char *host, const char *path);

int CatFilterLoad(const char *fname);

class TCatFilter {
private:
    ui32 DefFilter[2];
    ui32 *RootNode;
    ui32 *Filter;
    TCatAttrsPtr NullReturn;
    const char* FirstChar;
    TMappedFile FilterMap;
    bool IsMapped;

    ui32 *FindChild(ui32 *Node, const char *key) const;
    void FreeFilter();
    TCatAttrsPtr GetReturnedAttrs(const ui32* attrs) const;

public:
    TCatFilter();
    ~TCatFilter();

    TCatAttrsPtr Find0(const char *relurl) const;
    TCatAttrsPtr Find(const char *url) const;
    TCatAttrsPtr Find2(const char *host, const char *path
        , TStringBuf scheme = TStringBuf()) const;

    int Load(const char *fname);
    int Map(const char *fname);
};
