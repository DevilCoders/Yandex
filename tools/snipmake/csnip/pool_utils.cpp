#include "pool_utils.h"

#include <tools/snipmake/common/common.h>

#include <library/cpp/html/entity/htmlentity.h>
#include <library/cpp/json/easy_parse/json_easy_parser.h>
#include <library/cpp/string_utils/old_url_normalize/url.h>

#include <util/charset/wide.h>
#include <util/memory/tempbuf.h>
#include <util/random/random.h>
#include <util/stream/buffered.h>
#include <util/stream/file.h>
#include <util/string/printf.h>
#include <util/string/split.h>
#include <util/string/subst.h>
#include <util/string/vector.h>

namespace NSnippets {

    const TString CANDIDATE_ALGO_PREFIX = "retext";

    TPoolMaker::TPoolMaker(bool poolGeneration)
      : PoolGeneration(poolGeneration)
    {
    }

    bool TPoolMaker::Skip(const TString& query, const TString& url)
    {
        if (!PoolGeneration) {
            return false;
        }
        return !ProcessedQDPairs.insert(WideToUTF8(to_lower(UTF8ToWide(query))) + url).second;
    }

// -------------------------------------------------------------------------------------------------------------

    static const float EQUAL_PAIR_WEIGHT = 0.5;
    static const float BETTER_PAIR_WEIGHT = 1;
    static const TString BETTER_MARK = "BETTER";
    static const TString EQUAL_MARK = "EQUALLY";
    static const TString WORSE_MARK = "WORSE";
    static const TString LEFT = "left";
    static const TString RIGHT = "right";
    static const TString EQUAL = "eq";
    static const TString BOTH = "both";
    static const TString INFORMATIVENESS = "inf";
    static const TString CONTENT = "cont";
    static const TString READABILITY = "read";

    static TString HtmlDecodeUtf8(const TString& str) {
        TTempArray<char> tempBuf((str.size() + 1) * 2);
        size_t bufLen = HtEntDecodeToUtf8(CODES_UTF8, str.data(), str.size(), tempBuf.Data(), tempBuf.Size());
        return TString(tempBuf.Data(), bufLen);
    }

    bool TAssessorDataManager::IsEmpty() const
    {
        return QDPairCandidateIndices.empty();
    }

    void TAssessorDataManager::Init(const TString& assessorDataFileName)
    {
        TBuffered<TUnbufferedFileInput> in(8192, assessorDataFileName);
        TVector<TString> lines;
        TString line;
        TVector<TString> linesForJson;
        TString lineForJson;
        while (in.ReadLine(line)) {
            lines.push_back(line);
            // fast but risky, should be improved
            if (line == "        {") {
                lineForJson = line + "\n";
            } else {
                lineForJson += line + "\n";
            }
            if (line == "        }, ") {
                linesForJson.push_back(lineForJson);
                lineForJson.clear();
                continue;
            }
        }
        if (!linesForJson.empty()) {
            lines.clear();
            NJson::TJsonParser jsonParser;
            jsonParser.AddField("/request", true);
            jsonParser.AddField("/url", true);
            jsonParser.AddField("/region", true);
            jsonParser.AddField("/first_snippet/title", true);
            jsonParser.AddField("/first_snippet/text", true);
            jsonParser.AddField("/second_snippet/title", true);
            jsonParser.AddField("/second_snippet/text", true);
            jsonParser.AddField("/values/informativity", true);
            jsonParser.AddField("/values/content_richness", true);
            jsonParser.AddField("/values/readability", true);
            for (size_t num = 0; num < linesForJson.size(); ++num) {
                TString parsedLine = jsonParser.ConvertToTabDelimited(linesForJson[num]);
                if (!parsedLine.empty()) {
                    parsedLine = parsedLine.substr(1);
                    lines.push_back(parsedLine);
                }
            }
        }
        for (size_t num = 0; num < lines.size(); ++num) {
            TString line = lines[num];
            TVector<TString> fields;
            StringSplitter(line).Split('\t').SkipEmpty().Collect(&fields);
            if (fields.size() > 4 && fields[3].find("@@") != TString::npos && fields[3].find("://") == TString::npos) {
                //remove extra buggy field that looks like 37.617671,55.755768@@1.001816,0.367972@@z=11
                //after ang fix newer pools won't have this buggy field
                fields.erase(fields.begin() + 3);
            }
            if (fields.size() == 3) {
                continue;
            }
            if (fields.size() == 10) {
                const TString& query = fields[0];
                const TString& url = fields[1];
                TString qdPair = GetQDPairText(query, url);
                int leftCandIndex = InsertCandidate(qdPair, fields[4], fields[3]);
                int rightCandIndex = InsertCandidate(qdPair, fields[6], fields[5]);
                if (fields[7].find(INFORMATIVENESS) == TString::npos)
                     fields[7] = INFORMATIVENESS + ":" + fields[7];
                if (fields[8].find(CONTENT) == TString::npos)
                     fields[8] = CONTENT + ":" + fields[8];
                if (fields[9].find(READABILITY) == TString::npos)
                     fields[9] = READABILITY + ":" + fields[9];
                QDPairCandidatePairMarks[qdPair][std::make_pair(leftCandIndex, rightCandIndex)].push_back(fields[7] + "\t" + fields[8] + "\t" + fields[9]);
                continue;
            }
            if (fields.size() != 13) {
                Cerr << "ANG pool record format is wrong. Must contain 13 fields" << Endl << line << Endl;
                continue;
            }
            const TString& query = fields[0];
            const TString& url = fields[3];
            TString qdPair = GetQDPairText(query, url);
            int leftCandIndex = InsertCandidate(qdPair, fields[5], fields[4]);
            int rightCandIndex = InsertCandidate(qdPair, fields[7], fields[6]);
            QDPairCandidatePairMarks[qdPair][std::make_pair(leftCandIndex, rightCandIndex)].push_back(fields[8]);
        }
    }

    size_t TAssessorDataManager::InsertCandidate(const TString& qdPair, const TString& candidate, const TString& title)
    {
        size_t index;
        const auto it = QDPairCandidateIndices[qdPair].find(std::make_pair(candidate, title));
        if (it == QDPairCandidateIndices[qdPair].end()) {
            index = QDPairCandidateIndices[qdPair].size();
            QDPairCandidateIndices[qdPair][std::make_pair(candidate, title)] = index;
        } else {
            index = it->second;
        }
        return index;
    }

    TString TAssessorDataManager::GetRetextExp(const TString& query, const TString& url) const
    {
        TString exp;
        TString qdPair = GetQDPairText(query, url);
        const auto candidateIndicesIt = QDPairCandidateIndices.find(qdPair);
        if (candidateIndicesIt != QDPairCandidateIndices.end() && candidateIndicesIt->second.size() > 1) {
            const auto& candidateIndices = candidateIndicesIt->second;
            for (const auto& candidate : candidateIndices) {
                if (exp.size()) {
                    exp += ",";
                }
                exp += Sprintf("%s%zu", CANDIDATE_ALGO_PREFIX.data(), candidate.second) + "=" + GetCleanCandidateText(candidate.first.first);
                exp += ",";
                exp += Sprintf("title%s%zu", CANDIDATE_ALGO_PREFIX.data(), candidate.second) + "=" + GetCleanCandidateText(candidate.first.second);
            }
        }
        return exp;
    }

    TString TAssessorDataManager::GetCleanCandidateText(const TString& candText) const
    {
        TString ret = HtmlDecodeUtf8(candText);
        SubstGlobal(ret, ',', ' ');
        SubstGlobal(ret, "<strong>", "");
        SubstGlobal(ret, "</strong>", "");
        SubstGlobal(ret, "<b>", "");
        SubstGlobal(ret, "</b>", "");
        return ret;
    }

    TString TAssessorDataManager::GetQDPairText(const TString& query, const TString& url) const
    {
        return WideToUTF8(to_lower(UTF8ToWide(query))) + " " + HtmlDecodeUtf8(StrongNormalizeUrl(url));
    }

// -------------------------------------------------------------------------------------------------------------

    MatrixnetDataWriter::MatrixnetDataWriter(const TAssessorDataManager& assessorDataManager)
      : FeaturesTxtOut(new TFixedBufferFileOutput("features.txt"))
      , FeaturesPairsTxtOut(new TFixedBufferFileOutput("features.txt.pairs"))
      , AssessorDataManager(assessorDataManager)
      , AllMarkPairs(0)
      , ProcessedMarkPairs(0)
      , WrittenLineNumber(0)
      , QDPairIndex(0)
    {
    }

    void MatrixnetDataWriter::Write(const TQDPairCandidates& qdPairCandidates)
    {
        for (TQDPairCandidates::const_iterator candidatesIt = qdPairCandidates.begin();
             candidatesIt != qdPairCandidates.end(); ++candidatesIt) {
            const TString& qdPair = candidatesIt->first;
            THashMap<size_t, size_t> candidateFileIndices;
            size_t index = 0;
            for (THashMap<size_t, TReqSnip>::const_iterator it = candidatesIt->second.begin();
                 it != candidatesIt->second.end(); ++it) {
                WriteCandidate(qdPair, it->second);
                candidateFileIndices[it->first] = index + WrittenLineNumber;
                ++index;
            }
            auto candidatePairMarksIt = AssessorDataManager.QDPairCandidatePairMarks.find(qdPair);
            Y_ASSERT(candidatePairMarksIt != AssessorDataManager.QDPairCandidatePairMarks.end());

            typedef THashMap<std::pair<size_t, size_t>, TVector<TString> > TPairMarks;
            const TPairMarks& pairMarks = candidatePairMarksIt->second;
            for (TPairMarks::const_iterator pairMarksIt = pairMarks.begin(); pairMarksIt != pairMarks.end(); ++pairMarksIt) {
                THashMap<size_t, size_t>::const_iterator leftCandIndexIt = candidateFileIndices.find(pairMarksIt->first.first);
                THashMap<size_t, size_t>::const_iterator rightCandIndexIt = candidateFileIndices.find(pairMarksIt->first.second);
                ++AllMarkPairs;
                if (leftCandIndexIt == candidateFileIndices.end() || rightCandIndexIt == candidateFileIndices.end()) {
                    continue;
                }
                ++ProcessedMarkPairs;
                size_t markIndex = RandomNumber<size_t>(pairMarksIt->second.size());
                WriteFeaturesPairs(leftCandIndexIt->second, rightCandIndexIt->second, pairMarksIt->second[markIndex]);
            }
            WrittenLineNumber += candidatesIt->second.size();
            ++QDPairIndex;
        }
    }

    void MatrixnetDataWriter::WriteCandidate(const TString& /*qdPair*/, const TReqSnip& snippet)
    {
        *FeaturesTxtOut << QDPairIndex << "\t1\t" << snippet.SnipText[0].Text << "\t1";
        TVector<TString> factors;
        StringSplitter(snippet.FeatureString).Split(' ').SkipEmpty().Collect(&factors);
        for (size_t i = 0; i < factors.size(); ++i) {
            *FeaturesTxtOut << "\t" << factors[i].substr(factors[i].find(":") + 1);
        }
        *FeaturesTxtOut << Endl;
    }

    float MatrixnetDataWriter::GetWeight(int informativenessValue, int contentValue, int readabilityValue) const{
        return 1.0 * informativenessValue + 0.25 * contentValue + 0.0625 * readabilityValue;
    }

    void MatrixnetDataWriter::WriteFeaturesPairs(size_t leftCandNumber, size_t rightCandNumber, const TString& mark)
    {
        if (mark == EQUAL_MARK) {
            *FeaturesPairsTxtOut << leftCandNumber << "\t" << rightCandNumber << "\t" << EQUAL_PAIR_WEIGHT << Endl;
            *FeaturesPairsTxtOut << rightCandNumber << "\t" << leftCandNumber << "\t" << EQUAL_PAIR_WEIGHT << Endl;
        } else if (mark == WORSE_MARK) {
            *FeaturesPairsTxtOut << rightCandNumber << "\t" << leftCandNumber<< "\t" << BETTER_PAIR_WEIGHT << Endl;
        } else if (mark == BETTER_MARK) {
            *FeaturesPairsTxtOut << leftCandNumber << "\t" << rightCandNumber << "\t" << BETTER_PAIR_WEIGHT << Endl;
        } else {
            int informativenessValue = 0;
            if (mark.find(INFORMATIVENESS + ":" + LEFT) != TString::npos) {
                informativenessValue = 1;
            } else if (mark.find(INFORMATIVENESS + ":" + RIGHT) != TString::npos) {
                informativenessValue = -1;
            } else if (mark.find(INFORMATIVENESS + ":" + EQUAL) != TString::npos || mark.find(INFORMATIVENESS + ":" + BOTH) != TString::npos){
                informativenessValue = 0;
            } else {
                Cerr << "Pool record format is wrong. Must contain informativeness info" << Endl;
                return;
            }
            int contentValue = 0;
            if (mark.find(CONTENT + ":" + LEFT) != TString::npos) {
                contentValue = 1;
            } else if (mark.find(CONTENT + ":" + RIGHT) != TString::npos) {
                contentValue = -1;
            } else if (mark.find(CONTENT + ":" + EQUAL) != TString::npos || mark.find(CONTENT + ":" + BOTH) != TString::npos){
                contentValue = 0;
            } else {
                Cerr << "Pool record format is wrong. Must contain content info." << Endl;
                return;
            }
            int readabilityValue = 0;
            if (mark.find(READABILITY + ":" + LEFT) != TString::npos) {
                readabilityValue = 1;
            } else if (mark.find(READABILITY + ":" + RIGHT) != TString::npos) {
                readabilityValue = -1;
            } else if (mark.find(READABILITY + ":" + EQUAL) != TString::npos || mark.find(READABILITY + ":" + BOTH) != TString::npos){
                readabilityValue = 0;
            } else {
                Cerr << "Pool record format is wrong. Must contain readability info." << Endl;
                return;
            }
            float weight = GetWeight(informativenessValue, contentValue, readabilityValue);
            if (weight > 0)
                *FeaturesPairsTxtOut << leftCandNumber << "\t" << rightCandNumber << "\t" << weight << Endl;
            if (weight < 0)
                *FeaturesPairsTxtOut << rightCandNumber << "\t" << leftCandNumber << "\t" << -weight << Endl;
        }
    }

} //namespace NSnippets
