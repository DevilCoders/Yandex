#pragma once

#include <tools/snipmake/common/common.h>

#include <util/generic/ptr.h>

namespace NSnippets {

    class TSnip2SerpIter : public ISerpsIterator {

        TReqSerp Res;
        ISnippetsIterator* Iter;

    public:
        TSnip2SerpIter(ISnippetsIterator* iter)
          : Iter(iter)
        {
        }

        const TReqSerp& Get() const override {
            return Res;
        }

        bool Next() override {
            if (!Iter->Next())
                return false;
            Res.Snippets.resize(1);
            TReqSnip& snip = Res.Snippets[0];
            snip = Iter->Get();
            Res.Id++;
            Res.Query = snip.Query;
            Res.B64QTree = snip.B64QTree;
            Res.Region = snip.Region;
            Res.RichRequestTree = snip.RichRequestTree;
            return true;
       }

        ~TSnip2SerpIter() override {}
    };
}
