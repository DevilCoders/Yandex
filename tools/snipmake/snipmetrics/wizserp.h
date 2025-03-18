#pragma once

#include <tools/snipmake/common/common.h>
#include <tools/snipmake/serpmetrics_xml_parser/metricsserp.h>

#include <kernel/qtree/richrequest/richnode.h>

#include <util/stream/input.h>
#include <util/stream/zlib.h>
#include <util/generic/string.h>

namespace NSnippets {

    class TWizReqIterator {
    private:
        TBufferedZLibDecompress Stream;
        TString Str, Req, Reg;
        TRichTreePtr Tree;
        TString B64QTree;
        size_t Index;
    public:
        TWizReqIterator(IInputStream* in);
        TRichTreePtr RichTree() const;
        TString GetB64QTree() const;
        const TString& Request() const;
        const TString& Region() const;
        size_t GetLineIndex() const {
            return Index;
        }
        bool Next();
    };

    class TSerpXmlIterator : public ISerpsIterator {
    private:
        TSerpXmlReader Serp;
        TWizReqIterator Trees;
        TReqSerp Current;
        size_t State;
    private:
        bool IsSerpValid(const TReqSerp* serp) const;
        const TReqSnip* MoveSerp();
    public:
        TSerpXmlIterator(IInputStream* serp, IInputStream* wiz);
        bool Next() override;
        const TReqSerp& Get() const override;
    };

}
