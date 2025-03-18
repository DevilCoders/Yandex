#pragma once
#include "job.h"

#include <tools/snipmake/snippet_xml_parser/cpp_writer/snippet_dump_xml_writer.h>

#include <util/generic/string.h>
#include <util/stream/output.h>
#include <util/generic/ptr.h>
#include <util/stream/file.h>

namespace NSnippets {

    class TPassageReply;

    struct TXmlOutput : IOutputProcessor {
        THolder<TUnbufferedFileOutput> File;
        TSnippetDumpXmlWriter Writer;

        void Start();
        TXmlOutput(const TString& name);
        TXmlOutput(IOutputStream& out = Cout);
        void PrintXml(const TString& req, const TString& qtree, const TString& url, const TPassageReply& res, ui64 region);
        void Process(const TJob& job, bool exp);
        void Process(const TJob& job) override;
        void Complete() override;
    };

    struct TXmlDiffOutput : IOutputProcessor
    {
        TXmlOutput Left;
        TXmlOutput Right;

        TXmlDiffOutput(const TString& prefix);
        void Process(const TJob& job) override;
        void Complete() override;
    };

}
