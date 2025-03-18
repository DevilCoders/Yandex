#pragma once

#include "tree.h"
#include <library/cpp/html/face/onchunk.h>
#include <library/cpp/html/storage/storage.h>
#include <util/generic/ptr.h>

class IOutputStream;

namespace NHtmlTree {
    /// @todo use chunks region / iterator
    class TBuildTreeParserResult
       : public NHtml::TParserResult {
    public:
        TBuildTreeParserResult(TTree* doc);
        ~TBuildTreeParserResult() override;
        THtmlChunk* OnHtmlChunk(const THtmlChunk& chunk) override;

    private:
        class TImpl;
        THolder<TImpl> Impl;
    };

    void DumpTree(IOutputStream& os, const TTree* t);
    void DumpXPaths(IOutputStream& os, const TTree* t, const char* outputDocPrefix = nullptr);

}
