#pragma once

#include <tools/snipmake/common/common.h>

#include <library/cpp/logger/priority.h>
#include <library/cpp/xml/parslib/xmlsax.h>

#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NSnippets {

    struct TSnippetsParser;
    class TSnippetsXmlIterator : public ISnippetsIterator {
    private:
        IInputStream* Input;
        THolder<TSnippetsParser> Parser;
        size_t Index;
    public:
        TSnippetsXmlIterator(IInputStream* inp, bool deserializeQTree = true);
        bool Next() override;
        const TReqSnip& Get() const override;
        ~TSnippetsXmlIterator() override;
    };
}
