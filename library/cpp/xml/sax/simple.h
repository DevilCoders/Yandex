#pragma once

#include <util/generic/strbuf.h>

class IInputStream;

namespace NXml {
    class ISimpleSaxHandler {
    public:
        struct TAttr {
            TStringBuf Name;
            TStringBuf Value;
        };
        virtual ~ISimpleSaxHandler() {
        }
        virtual void OnStartElement(const TStringBuf& name, const TAttr* attrs, size_t count) = 0;
        virtual void OnEndElement(const TStringBuf& name) = 0;
        virtual void OnText(const TStringBuf& data) = 0;
    };

    void Parse(IInputStream& in, ISimpleSaxHandler* h);
}
