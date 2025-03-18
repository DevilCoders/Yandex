#pragma once
#include "job.h"
#include "pool_utils.h"

#include <tools/snipmake/snippet_xml_parser/cpp_writer/snippet_dump_xml_writer.h>

#include <util/generic/string.h>
#include <util/stream/output.h>

namespace NSnippets {

    struct TDumpCandidatesOutput : IOutputProcessor {
        TSnippetDumpXmlWriter XmlOut;
        bool PoolGenerator = false;
        bool WithoutAng = false;
        size_t ProcessedQDPairCount = 0;

        TDumpCandidatesOutput(const TString& file, bool poolGenerator, bool withoutAng);
        void Process(const TJob& job) override;
        void Complete() override;
    };

    struct TImgDumpOutput : IOutputProcessor {
        void Process(const TJob& job) override;
    };

    struct TUpdateCandidatesOutput : IOutputProcessor {
        const TAssessorDataManager& AssessorDataManager;
        TQDPairCandidates QDPairCandidates;

        TUpdateCandidatesOutput(const TAssessorDataManager& assessorDataManager);
        void Process(const TJob& job) override;
        void Complete() override;
    };

}
