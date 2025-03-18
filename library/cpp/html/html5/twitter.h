#pragma once

#include "node.h"
#include "output.h"

#include <util/generic/vector.h>
#include <util/generic/ptr.h>

namespace NHtml5 {
    //! https://wiki.yandex-team.ru/robot/indexing/docs/htmlparser/dev/exceptionalchunks
    class TTwitterConverter {
    public:
        TTwitterConverter(TOutput* output);

        void MaybeChangeSubtree(TNode* node);

    private:
        bool IsExceptionalNode(const TNode* node) const;
        bool ProcessATag(TNode* aElement);

    private:
        TOutput* const ParserOutput_;
        std::function<TNode**(size_t)> Alloc_;
        TVector<int> NewIndexes_;
    };

    THolder<TTwitterConverter> CreateTreeConverter(const TStringBuf& url, TOutput* output);

}
