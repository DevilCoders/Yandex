#include "output_dump.h"
#include "html_hilite.h"
#include "lines_count.h"

#include <tools/snipmake/common/common.h>

#include <kernel/snippets/util/xml.h>

#include <util/charset/wide.h>
#include <util/generic/vector.h>
#include <util/stream/output.h>
#include <util/stream/str.h>
#include <util/string/cast.h>
#include <library/cpp/string_utils/url/url.h>

namespace NSnippets {

    static const size_t POOL_QDPAIRS_NUMBER = 12000;

    TDumpCandidatesOutput::TDumpCandidatesOutput(const TString& file, bool poolGenerator, bool withoutAng)
        : XmlOut(Cout)
        , PoolGenerator(poolGenerator)
        , WithoutAng(withoutAng)
    {
        if (!WithoutAng) {
            XmlOut.Start();
            XmlOut.StartPool(file);
        }
    }

    void TDumpCandidatesOutput::Process(const TJob& job)
    {
        if (PoolGenerator && ProcessedQDPairCount == POOL_QDPAIRS_NUMBER) {
            return;
        }

        TStringInput in(job.Reply.GetSnippetsExplanation());
        TString cand;
        bool isQDPairOpened = false;
        while (in.ReadLine(cand)) {
            TReqSnip snippet;
            snippet.TitleText = job.Reply.GetTitle();
            ParseDumpsnipcandsCandIntoTSnippet(cand, &snippet);
            snippet.TitleText = RehighlightAndHtmlEscape(snippet.TitleText);
            snippet.Lines = GetLinesCount(cand);

            if (!isQDPairOpened) {
                isQDPairOpened = true;
                ++ProcessedQDPairCount;
                if (WithoutAng) {
                    Cout << job.UserReq << "\t" << AddSchemePrefix(job.ArcUrl) << "\t" << job.Region;
                } else {
                    XmlOut.StartQDPair(job.UserReq, AddSchemePrefix(job.ArcUrl), "1", ToString(job.Region), job.ContextData.GetId());
                }
            }
            if (WithoutAng) {
                Cout << "\t" << snippet.TitleText << "\t";
                bool start = true;
                for (TVector<TSnipFragment>::const_iterator fragmentIter = snippet.SnipText.begin(); fragmentIter != snippet.SnipText.end(); ++fragmentIter) {
                    if (!start)
                        Cout << " ... ";
                    start = false;
                    Cout << EncodeTextForXml10(WideToUTF8(RehighlightAndHtmlEscape(fragmentIter->Text)));
                }
            } else {
                XmlOut.WriteSnippet(snippet);
            }
        }
        if (isQDPairOpened) {
            if (WithoutAng) {
                Cout << "\t" << WideToUTF8(RehighlightAndHtmlEscape(UTF8ToWide(job.Reply.GetHilitedUrl()))) << Endl;
            } else {
                XmlOut.FinishQDPair();
            }
        }
    }

    void TDumpCandidatesOutput::Complete()
    {
        if (!WithoutAng) {
            XmlOut.FinishPool();
            XmlOut.Finish();
        }
    }


    void TImgDumpOutput::Process(const TJob& job) {
        TStringInput in(job.Reply.GetSnippetsExplanation());
        TString cand;
        while (in.ReadLine(cand)) {
            TReqSnip snippet;
            ParseDumpsnipcandsCandIntoTSnippet(cand, &snippet);
            Cout << job.ContextData.GetId() << "\t" << snippet.ImgUH << "\t" << snippet.FeatureString << Endl;
        }
    }


    TUpdateCandidatesOutput::TUpdateCandidatesOutput(const TAssessorDataManager& assessorDataManager)
      : AssessorDataManager(assessorDataManager)
    {
    }

    void TUpdateCandidatesOutput::Process(const TJob& job) {
        const TString& explanation = job.Reply.GetSnippetsExplanation();
        if (explanation.size() == 0) {
            return;
        }

        TStringInput in(explanation);
        TString cand;
        TString qdPair = job.JobEnv.AssessorDataManager.GetQDPairText(job.UserReq, job.ArcUrl);
        if (QDPairCandidates.find(qdPair) != QDPairCandidates.end()) {
            QDPairCandidates[qdPair].clear();
        }
        while (in.ReadLine(cand)) {
            TReqSnip snippet;
            ParseDumpsnipcandsCandIntoTSnippet(cand, &snippet);
            if (!snippet.Algo.StartsWith(CANDIDATE_ALGO_PREFIX)) {
                continue;
            }
            snippet.MergeFragments();
            QDPairCandidates[qdPair][FromString<size_t>(snippet.Algo.substr(CANDIDATE_ALGO_PREFIX.size()))] = snippet;
        }
    }

    void TUpdateCandidatesOutput::Complete() {
        MatrixnetDataWriter writer(AssessorDataManager);
        writer.Write(QDPairCandidates);

        Cout << "Processed marks: " << writer.AllMarkPairs << Endl;
        Cout << "Lost marks: " << writer.AllMarkPairs - writer.ProcessedMarkPairs << "(" << 100.0*(writer.AllMarkPairs - writer.ProcessedMarkPairs) / writer.AllMarkPairs << "%)" << Endl;
    }

} //namespace NSnippets
