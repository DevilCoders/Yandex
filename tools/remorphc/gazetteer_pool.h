#pragma once

#include <kernel/gazetteer/gazetteer.h>

#include <util/generic/hash.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/stream/output.h>

namespace NRemorphCompiler {

class TGazetteerPool: public TSimpleRefCount<TGazetteerPool> {
private:
    THashMap<TString, TAutoPtr<NGzt::TGazetteer>> Pool;
    IOutputStream* Log;

public:
    explicit TGazetteerPool(IOutputStream* log = nullptr);

    const NGzt::TGazetteer* Add(const TString& path);
};

typedef TIntrusivePtr<TGazetteerPool> TGazetteerPoolPtr;

} // NRemorphCompiler
