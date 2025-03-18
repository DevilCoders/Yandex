#pragma once

#include <libxml/xmlwriter.h>
#include <tools/snipmake/common/common.h>
#include <util/generic/string.h>
#include <util/string/vector.h>
#include <util/string/util.h>

namespace NSnippets
{
    class TSnippetDumpXmlWriter {
    private:
        IOutputStream& Out;

        void WriteFeatures(xmlTextWriterPtr writer, const TString& featuresString);
        void WriteMark(xmlTextWriterPtr writer, const TSnipMark& mark);
        void WriteFragment(xmlTextWriterPtr writer, const TSnipFragment& fragment);
        void WriteSnippet(xmlTextWriterPtr writer, const TReqSnip& snippet);

    public:
        TSnippetDumpXmlWriter(IOutputStream& out)
            : Out(out)
        {
        }

        void Start();
        void StartPool(const TString& name);
        void Finish();
        void FinishPool();

        void WriteSnippet(const TReqSnip& snippet);
        void StartQDPair(const TString& query, const TString& url, const TString& relevance,
                         const TString& region, const TString& richTreee);
        void FinishQDPair();
    };
}
