#pragma once

#include <library/cpp/html/face/onchunk.h>
#include <library/cpp/xml/doc/xmldoc.h>
#include <library/cpp/xml/document/xml-document.h>
#include <util/generic/ptr.h>

namespace NHtmlTree {
    class TBuildXmlTreeParserResult: public IParserResult {
    public:
        TBuildXmlTreeParserResult(NXml::TDocument* doc);
        ~TBuildXmlTreeParserResult() override;
        THtmlChunk* OnHtmlChunk(const THtmlChunk& chunk) override;

    private:
        class TImpl;
        THolder<TImpl> Impl;
    };

}
