#include <tools/snipmake/contentstats/util/kiwi.h>
#include <yweb/structhtml/htmlstatslib/htmlstatslib.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/html/pcdata/pcdata.h>
#include <util/generic/string.h>
#include <util/string/cast.h>
#include <util/stream/file.h>
#include <util/string/split.h>
#include <util/string/strip.h>
#include <util/generic/ptr.h>
#include <utility>

using TMarkKey = std::pair<TString, ui64>;
enum EMark {
    NONE,
    TEMPLATE,
    UNIQUE
};
using TMarks = THashMap<TMarkKey, EMark>;
using TUrlSet = THashSet<TString>;
using THashValues = THashSet<ui64>;

struct TUrlData
{
    TString Group;
    TVector<ui64> MarkedHashes;
};

using TUrlDataPtr = TSimpleSharedPtr<TUrlData>;
using TUrlDataMap = THashMap<TString, TUrlDataPtr>;

void ReadGroupedUrls(const TString& fileName, TUrlDataMap& urlDataMap)
{
    TFileInput inf(fileName, 1<<16);
    TString line;
    while (inf.ReadLine(line)) {
        TStringBuf buf(line);
        StripString(buf, buf);
        TStringBuf url, group;
        Split(buf, '\t', group, url);
        TUrlDataPtr datum(new TUrlData());
        datum->Group = TString{group};
        urlDataMap[TString{url}] = datum;
    }
}

void LoadMarks(const TString& fileName, TMarks& perUrl, TUrlSet& sealedUrls)
{
    TFileInput inf(fileName, 1<<16);
    TString line;
    int numJsonAccepted = 0;
    int numJsonTotal = 0;
    while (inf.ReadLine(line)) {
        ++numJsonTotal;
        TStringInput jsonLine(line);
        NJson::TJsonValue json;
        NJson::ReadJsonTree(&jsonLine, &json, false);
        const NJson::TJsonValue* jUrl = json.GetValueByPath("url", '.');
        const NJson::TJsonValue* jHash = json.GetValueByPath("sent.hash", '.');
        const NJson::TJsonValue* jMark = json.GetValueByPath("state", '.');
        const NJson::TJsonValue* jCmd = json.GetValueByPath("cmd", '.');
        TString url;
        if (jUrl) {
            url = DecodeHtmlPcdata(jUrl->GetString());
        }
        if (!url) {
            continue;
        }
        if (!!jCmd) {
            if (jCmd->GetString() == "seal") {
                sealedUrls.insert(url);
            } else {
                sealedUrls.erase(url);
            }
            continue;
        }
        if (!jHash || !jMark) {
            continue;
        }
        sealedUrls.erase(url);
        ui64 hash = FromString<ui64>(jHash->GetString());
        TString sMark = jMark->GetString();
        EMark mark = NONE;
        if (sMark == "good") {
            mark = UNIQUE;
        }
        else if (sMark == "bad") {
            mark = TEMPLATE;
        }
        perUrl[std::make_pair(url, hash)] = mark;
        ++numJsonAccepted;
    }
    Cerr << "Read " << fileName << ": " << numJsonTotal << " lines, " << numJsonAccepted << " marks" << Endl;
}


NHtmlStats::THashGroup* LookupHashGroup(const TString& group, const NHtmlStats::THashGroups& groups)
{
    NHtmlStats::THashGroups::const_iterator groupIter = groups.find(group);
    if (groupIter != groups.end()) {
        return groupIter->second.Get();
    }

    Cerr << "No hashes found for group " << group << Endl;
    return nullptr;
}

int main(int argc, const char* argv[])
{
    using namespace NHtmlStats;

    Y_UNUSED(argc);
    Y_UNUSED(argv);

    TUrlGrouper grouper;

    TUrlDataMap urlData;
    ReadGroupedUrls("url_groups.txt", urlData);

    THashGroups hashGroups0;
    THashGroups hashGroups1;

    {
        TFileInput inf("learned_freqs_0.txt", 1<<20);
        LoadHashes(inf, hashGroups0);
    }

    {
        TFileInput inf("learned_freqs.txt", 1<<20);
        LoadHashes(inf, hashGroups1);
    }

    TMarks perUrlMarks;
    TUrlSet sealedUrls;
    LoadMarks("votes.json", perUrlMarks, sealedUrls);

    for (const auto& mark : perUrlMarks) {
        const TString& url = mark.first.first;
        const ui64 hash = mark.first.second;
        if (!urlData.contains(url)) {
            Cerr << "mark for non-existent URL: " << url << Endl;
            continue;
        }
        urlData[url]->MarkedHashes.push_back(hash);
        Cerr << "added hash for " << url << Endl;
    }

    NHtmlStats::THtmlParser parser;

    TFileInput docsStream("sample_docs.protobin", 1<<20);
    NHtmlStats::TProtoStreamIterator iter(&docsStream);

    int recnum = 0;
    NHtmlStats::TProtoDoc doc;
    while (iter.Next(doc)) {
        ++recnum;

        const TString url = doc.Url;

        if (!doc.Html || !url || doc.Encoding < 0 || doc.Encoding >= CODES_MAX) {
            TString id = url;
            if (!id) {
                id = ToString<int>(recnum);
            }
            continue;
        }

        if (!sealedUrls.contains(url)) {
            continue;
        }
        Cerr << "Sealed URL: " << url << Endl;

        TUrlDataPtr urlDatum;
        {
            TUrlDataMap::iterator ii = urlData.find(url);
            if (ii != urlData.end()) {
                urlDatum = ii->second;
            }
        }

        if (!urlDatum) {
            Cerr << "No URL data" << Endl;
            continue;
        }

        Cerr << "Num. marks for URL: " << urlDatum->MarkedHashes.size() << Endl;

        THashGroup* hashes0 = LookupHashGroup(urlDatum->Group, hashGroups0);
        THashGroup* hashes1 = LookupHashGroup(urlDatum->Group, hashGroups1);
        if (!hashes0 || !hashes1) {
            Cerr << "No hash frequency data for URL: " << url << Endl;
            continue;
        }

        TSentenceEvaluator evaluator0(*hashes0);
        TSentenceEvaluator evaluator1(*hashes1);
        parser.Parse(url, doc.Html, doc.Encoding, &evaluator0, TParserOptions().DoReplaceNumbers());
        parser.Parse(url, doc.Html, doc.Encoding, &evaluator1, TParserOptions().DoReplaceNumbers());

        size_t numSents = evaluator0.Sentences.size();
        if (!numSents) {
            Cerr << "No sentences in the document: " << url << Endl;
            continue;
        }
        if (numSents != evaluator1.Sentences.size()) {
            Cerr << "Parsing options affect sentence boundaries for " << url << Endl;
            continue;
        }

        size_t numCorrect = 0;
        size_t numThisUrlMarks = 0;
        size_t numSignificant = 0;

        THashValues sentHashVals;

        for (size_t i = 0; i < numSents; ++i) {
            const auto& sent0 = evaluator0.Sentences[i];
            const auto& sent1 = evaluator1.Sentences[i];

            sentHashVals.insert(sent0.Hash);

            // Extract the assessment made with the reference sample
            EMark mark = NONE;
            if (sent0.Freq < .03) {
                mark = UNIQUE;
            } else if (sent0.Freq > .8) {
                mark = TEMPLATE;
            }

            TMarkKey key(url, sent0.Hash);
            EMark explicitMark = NONE;
            if (perUrlMarks.contains(key)) {
                explicitMark = perUrlMarks[key];
                mark = explicitMark;
                ++numThisUrlMarks;
            }

            // Compute the assessment quality wrt a corrected sample
            if (sent1.Freq < .03 || sent1.Freq > .8 || mark != NONE) {
                //Cerr << sent1.Freq << "\t" << (int)mark << "\t" << (int)explicitMark << "\t" << sent1.Sent << Endl;
                ++numSignificant;
            }
            else {
                continue;
            }

            if (sent1.Freq > .8) {
                numCorrect += (mark == TEMPLATE);
            }
            else if (sent1.Freq < .03) {
                numCorrect += (mark == UNIQUE);
            }
            else {
                numCorrect += (mark == NONE);
            }
        }

        for (const ui64 h : urlDatum->MarkedHashes) {
            if (!sentHashVals.contains(h)) {
                Cerr << "WARN: mark " << h << " for group " << urlDatum->Group << " does not occur in " << doc.Url << Endl;
            }
        }

        if (numSignificant == 0) {
            continue;
        }

        //Cout << doc.Url << ":" << Endl;
        double quality = (double)numCorrect*100.0/(double)numSignificant;
        Cout << quality << "\t" << url << Endl;
        //Cout << "  sents: " << numSents << Endl;
        //Cout << "  group marks: " << numAssessed << Endl;
        //Cout << "  url marks: " << numThisUrlMarks << Endl;
        //Cout << "  significant sents: " << numSignificant << Endl;
        //Cout << "    correct: " << correct << ", " << Sprintf("%.02f%%", (double)correct*100.0/(double)numSignificant) << Endl;
    }
}

