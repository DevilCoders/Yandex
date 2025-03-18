#pragma once

#include <util/generic/ptr.h>
#include <tools/snipmake/common/common.h>

namespace NSnippets {

    struct TSerpsParser;
    class TSerpsXmlIterator : public ISnippetsIterator {
    private:
        IInputStream* Input;
        THolder<TSerpsParser> Parser;
        size_t Index;
    public:
        TSerpsXmlIterator(IInputStream* inp);
        bool Next() override;
        const TReqSnip& Get() const override;
        ~TSerpsXmlIterator() override;
    };
}
