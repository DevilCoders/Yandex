#include "querybased.h"

#include <kernel/qtree/richrequest/richnode.h>
#include <kernel/indexer/face/inserter.h>
#include <library/cpp/charset/wide.h>

#include <util/memory/blob.h>
#include <util/string/split.h>

void ConvertWords2Lemmas(const TVector<TString>& words, TVector<TString>& textLemmas) {
    for(size_t i = 0; i < words.size(); ++i) {
        TWLemmaArray tmp;
        TUtf16String word = CharToWide(words[i], csYandex);
        NLemmer::AnalyzeWord(word.data(), word.size(), tmp, LI_BASIC_LANGUAGES);
        if (tmp.size() > 0) {
            textLemmas.push_back(WideToChar(TWtringBuf(tmp[0].GetText()), CODES_YANDEX));
        }
    }
}

template<class TWordParser>
void ReadFileLineByLine(const TString& fileName, TWordParser& parser) {
    TFileInput inFile(fileName);
    TString line;
    while (inFile.ReadLine(line)) {
        // skip empty lines
        if (!line)
            return;

        TVector<TString> words;
        StringSplitter(line).Split(' ').SkipEmpty().Collect(&words);
        parser.ProcessWords(words);
    }
}

class TWordsAndLemmasParser {
private:
    TVector<TString>& Words;
    TVector<TString>& Lemmas;
public:
    TWordsAndLemmasParser (TVector<TString>& words, TVector<TString>& lemmas)
        : Words(words)
        , Lemmas(lemmas)
    {}

    void ProcessWords(const TVector<TString>& words) {
        Y_VERIFY(words.size() == 2, "Size of word lemma pair must be equal 2 now %lu", (unsigned long)words.size());

        Words.push_back(words[0]);
        Lemmas.push_back(words[1]);
    }
};

void TQueryBasedProcessor::ProcessDirectText2(IDocumentDataInserter* inserter, const NIndexerCore::TDirectTextData2& directText, ui32 /*docId*/) {
    Prepare();
    LemmasCollector.Process(directText);
    if (CommercialDetector->GetResult())
        inserter->StoreErfDocAttr("IsComm", "1");

    if (PaymentsDetector->GetResult())
        inserter->StoreErfDocAttr("HasPayments", "1");

    if (SeoDetector->GetResult() || SeoExactDetector->GetResult())
        inserter->StoreErfDocAttr("IsSEO", "1");

    if (PornoDetector->GetResult() || Porno2WordsDetector->GetResult())
        inserter->StoreErfDocAttr("IsPorno", "1");
}

TQueryBasedProcessor::TQueryBasedProcessor(const TQueryFactorsConfig& config) {
    const bool exactWord = true;
    const bool inTitle = true;

    PaymentsDetector.Reset(BuildPaymentsDetector(config.PaymentsFileName, LemmasCollector));
    BuildPornoDetectors(config.PornoFileName, config.PornoFilePairs, LemmasCollector, PornoDetector, Porno2WordsDetector, config.UsePrepared);
    CommercialDetector.Reset(BuildSimpleDetector(config.CommFileName, LemmasCollector, !exactWord, !inTitle, config.UsePrepared));

    SeoDetector.Reset(BuildSimpleDetector(config.SeoFileName, LemmasCollector, !exactWord, inTitle, config.UsePrepared));

    if (config.UsePrepared)
        SeoExactDetector.Reset(BuildSimpleDetector(config.SeoFileNameExact, LemmasCollector, exactWord, inTitle, config.UsePrepared));
    else
        SeoExactDetector.Reset(BuildSimpleDetector(config.SeoFileName, LemmasCollector, exactWord, inTitle, config.UsePrepared));
}

void TQueryBasedProcessor::InitAdultDetector(const TString& adultFileName, TAdultPreparat& adultPreparat, bool usePrepared) {
    AdultDetector.Reset(BuildAdultDetector(adultFileName, adultPreparat, LemmasCollector, usePrepared).Release());
};

void TQueryBasedProcessor::Prepare() {
    CommercialDetector->Prepare();
    PaymentsDetector->Prepare();
    SeoDetector->Prepare();
    SeoExactDetector->Prepare();
    PornoDetector->Prepare();
    Porno2WordsDetector->Prepare();
    if (!!AdultDetector)
        AdultDetector->Prepare();
}

void ParseRichTree(const TRichRequestNode* n, NQueryFactors::TQueriesInfo& queries, size_t level, bool getOneWord, size_t distance) {
    const TString name = (!!n->GetText()) ? WideToChar(n->GetText(), CODES_YANDEX) : "";
    TRichRequestNode::TNodesVector::const_iterator it, end;

    if (level == 0) {
        Y_ASSERT(!name);                             // must have empty name

        if (!n->Children.empty()) {
            end = n->Children.end();
            for (it = n->Children.begin(); it != end; ++it)
                ParseRichTree((*it).Get(), queries, 1, getOneWord, 0);
        }
    } else if (level == 1) {
        if (!!name)                                 // not empty name
            return;                                 // skip categories

        size_t distance = n->OpInfo.Lo;             // simplification of "query language"
                                                    // take only one parameter
        end = n->Children.end();
        for (it = n->Children.begin(); it != end; ++it)
            ParseRichTree((*it).Get(), queries, 2, getOneWord, distance);
    } else if (level == 2) {
        if (getOneWord) {
            if (!!name) {
                NQueryFactors::TQueryInfo queryPreparat;
                queryPreparat.push_back(NQueryFactors::TQueryWordInfo(name, /*distance = */ 0));
                queries.push_back(queryPreparat);
            }
        } else {
            if (!name) {                                // fill only pairs
                it = n->Children.begin();
                end = n->Children.end();

                // parse 2 word together
                TRichRequestNode* node = (*it).Get();
                const TString name1 = (!!node->GetText()) ? WideToChar(node->GetText(), CODES_YANDEX) : "";

                ++it;
                Y_ASSERT(it != end);
                node = (*it).Get();
                const TString name2 = (!!node->GetText()) ? WideToChar(node->GetText(), CODES_YANDEX) : "";
                NQueryFactors::TQueryInfo queryPreparat;
                queryPreparat.push_back(NQueryFactors::TQueryWordInfo(name1, /*distance = */ 0));
                queryPreparat.push_back(NQueryFactors::TQueryWordInfo(name2, (ui32)distance));
                queries.push_back(queryPreparat);
            }
        }
    }

}

void MakeWordsAndLemmas4PornoDetectors(const TString& queryFileName, NQueryFactors::TQueriesInfo& queries, TVector<TString>& lemmas) {
    TString query;
    {
        TBlob vctQuery = TBlob::FromFile(queryFileName);
        query = TString(vctQuery.AsCharPtr(), 0, vctQuery.Length());
    }

    TCreateTreeOptions opts(LI_DEFAULT_REQUEST_LANGUAGES);
    TRichTreePtr RichTree = CreateRichTree(CharToWide(query, csYandex), opts);

    ParseRichTree(RichTree->Root.Get(), queries, 0, /* getOneWord = */ false, 0);

    NQueryFactors::TQueriesInfo tmpQueries;
    ParseRichTree(RichTree->Root.Get(), tmpQueries, 0, /* getOneWord = */ true, 0);

    TVector<TString> words;
    for (size_t i = 0; i < tmpQueries.size(); ++i) {
        words.push_back(tmpQueries[i][0].Word);
    }

    ConvertWords2Lemmas(words, lemmas);
}

class TPairInfoParser {
private:
    NQueryFactors::TQueriesInfo& QueriesStorage;

public:
    TPairInfoParser(NQueryFactors::TQueriesInfo& queries)
        : QueriesStorage(queries)
    {}

    void ProcessWords(const TVector<TString>& words) {
        Y_VERIFY(words.size() == 3, "Size of words must be equal 3, now %lu", (unsigned long)words.size());
        NQueryFactors::TQueryInfo queryPreparat;
        queryPreparat.push_back(NQueryFactors::TQueryWordInfo(words[0], /*distance = */ 0));
        queryPreparat.push_back(NQueryFactors::TQueryWordInfo(words[1], /*distance = */ FromString<ui32>(words[2])));

        QueriesStorage.push_back(queryPreparat);
    }
};

class TWordsParser {
private:
    TVector<TString>& Lemmas;

public:
    TWordsParser (TVector<TString>& lemmas)
        : Lemmas(lemmas)
    {}

    void ProcessWords(const TVector<TString>& words) {
        Y_VERIFY(words.size() == 1, "Size of word lemma pair must be equal 1 now %lu", (unsigned long)words.size());
        Lemmas.push_back(words[0]);
    }
};

void BuildPornoDetectors(
    const TString& queryFileName,
    const TString& queryFileNamePairs,
    TLemmasCollector& collector,
    THolder<IWordDetector>& simpleDetector,
    THolder<TDetectorUnion>& pairDetector,
    bool usePrepared)
{
    TVector<TString> lemmas;
    NQueryFactors::TQueriesInfo queries;
    if (!usePrepared) {
        MakeWordsAndLemmas4PornoDetectors(queryFileName, queries, lemmas);
    } else {
        TPairInfoParser piParser(queries);
        ReadFileLineByLine(queryFileNamePairs, piParser);

        TWordsParser wordParser(lemmas);
        ReadFileLineByLine(queryFileName, wordParser);
    }
    pairDetector.Reset(new TDetectorUnion(queries, collector));
    simpleDetector.Reset(new TSimpleDetector);

    for(size_t i = 0; i < lemmas.size(); ++i) {
        collector.AddLemmaAndDetector(lemmas[i], simpleDetector.Get());
    }
}

static void ParseRichTree(const TRichRequestNode* n, TVector<TString>& words, bool findExactWords, bool findInTitle) {
    const TString name = (!!n->GetText()) ? WideToChar(n->GetText(), CODES_YANDEX) : "";
    // not using title if find in title
    if (!!name) {
        bool wordInTitle = false;

        // change parsing richtree, title zone find in MiscOps
        if (!n->MiscOps.empty()) {
            const TRichRequestNode* miscOpNode = n->MiscOps.begin()->Get();
            const TString name = (!!miscOpNode->GetTextName()) ? WideToChar(miscOpNode->GetTextName(), CODES_YANDEX) : "";
            wordInTitle = (name == "title");
        }

        if (findInTitle != wordInTitle)
            return;

        TWordNode* wi = n->WordInfo.Get();
        bool inExactPart = false;
        if (wi != nullptr)
            inExactPart = wi->GetFormType() == fExactWord;

        if (inExactPart == findExactWords)        // push only if we want do it.
            words.push_back(name);

    } else {
        if (!n->Children.empty()){
            TRichRequestNode::TNodesVector::const_iterator it, end;
            end = n->Children.end();
            for (it = n->Children.begin(); it != end; ++it)
                ParseRichTree((*it).Get(), words, findExactWords, findInTitle);
        }
    }
}

void MakeWordsFromRichTree(const TString& queryFileName, TVector<TString>& words, bool findExactWords, bool findInTitle) {
    TString query;
    {
        TBlob vctQuery = TBlob::FromFile(queryFileName);
        query = TString(vctQuery.AsCharPtr(), 0, vctQuery.Length());
    }
    TCreateTreeOptions opts(LI_DEFAULT_REQUEST_LANGUAGES);
    TRichTreePtr richTree = CreateRichTree(CharToWide(query, csYandex), opts);
    ParseRichTree(richTree->Root.Get(), words, findExactWords, findInTitle);
}

IWordDetector* BuildSimpleDetector(const TString& queryFileName, TLemmasCollector& collector, bool findExactWords, bool findInTitle, bool usePrepared /* = false */) {
    TVector<TString> words;
    TVector<TString> lemmas;

    if (!usePrepared) {
        MakeWordsFromRichTree(queryFileName, words, findExactWords, findInTitle);
        ConvertWords2Lemmas(words, lemmas);
    } else {
        TWordsAndLemmasParser wlParser(words, lemmas);
        ReadFileLineByLine(queryFileName, wlParser);
    }

    THolder<IWordDetector> detector;

    if (findInTitle) {
        detector.Reset(new TTitleDetector);
    } else {
        detector.Reset(new TSimpleDetector);
    }

    if (!findExactWords) {
        TSet<TString> ss;                                // for unique lemmas
        for (size_t i = 0; i < lemmas.size(); ++i)
            ss.insert(lemmas[i]);

        for (TSet<TString>::iterator i = ss.begin(); i != ss.end(); ++i)
            collector.AddLemmaAndDetector((*i), detector.Get());

    } else {
        for (size_t i = 0; i < lemmas.size(); ++i)
            collector.AddLemmaAndDetector(lemmas[i], detector.Get(), words[i]);
    }
    return detector.Release();
}

IWordDetector* BuildPaymentsDetector(const TString& queryFileName, TLemmasCollector& collector) {
    TFileInput file(queryFileName);
    TString word;
    THolder<TSimpleDetector> detector(new TSimpleDetector);
    Y_ASSERT(detector.Get());
    while (file.ReadLine(word)) {
        if (word.size() > 1) {                                // delete leading "*"
            const char* szWithoutStar = word.data();
            ++szWithoutStar;
            // some fake lemma == word
            collector.AddLemmaAndDetector(TString(szWithoutStar), detector.Get());
        }
    }
    return detector.Release();
}


void ParsePornoConfigLine(const TString line, TVector<TString>& words, TString& query, float& weight) {
    TVector<TString> parts;
    Split(line, ":", parts);
    if (parts.size() != 2)
        ythrow yexception() << "bad porno config format: line '" << line << "'";

    Split(parts[0], " ", words);
    query = words[0];
    for (size_t i = 1; i < words.size(); ++i)
        query += " /+1 " + words[i];

    weight = FromString<float>(parts[1].data());
};

float GetPornoThreshold(const TString& line) {
    if (line.substr(0, 2) != "b=")
        ythrow yexception() << "bad porno config format in <" << line << ">";
    return FromString<float>(line.substr(2, line.size() - 2));
};

bool GetAdultLemmasFromString(const TString& line, float& wordWeight, TVector<TString>& lemmas) {
    TVector<TString> wordsTmp;
    TString query;
    wordWeight = 0.0;

    if (!line.size())
        return false;

    ParsePornoConfigLine(line, wordsTmp, query, wordWeight);

    if (!wordsTmp.size())
        return false;

    TCreateTreeOptions opts(LI_DEFAULT_REQUEST_LANGUAGES);
    TRichTreePtr RichTree = CreateRichTree(CharToWide(query, csYandex), opts);
    // get rich tree (only one or 2 words(lemmas) are detected)
    TVector<TString> words;
    ParseRichTree(RichTree->Root.Get(), words, false, false);

    ConvertWords2Lemmas(words, lemmas);

    return lemmas.size();
}

TAutoPtr<TAdultDetector> BuildAdultDetector(const TString& pornoConfig, TAdultPreparat& adultPreparat, TLemmasCollector& collector, bool usePrepared) {
    TFileInput pornoConfigFile(pornoConfig);
    TString line;
    pornoConfigFile.ReadLine(line);

    float threshold = 0.0f;
    try {
        threshold = GetPornoThreshold(line);
    } catch (yexception& ex) {
        ythrow yexception() << "Error: " << pornoConfig << " : " << ex.what();
    }

    adultPreparat.AdultThreshold = threshold;
    THolder<TAdultDetector> result(new TAdultDetector(adultPreparat));

    if (!usePrepared) {
        while (pornoConfigFile.ReadLine(line)) {
            TVector<TString> lemmas;
            float wordWeight = 0.0;

            if (!GetAdultLemmasFromString(line, wordWeight, lemmas))
                continue;

            result->AddWordsSequence2Detect(lemmas, wordWeight, collector);
        }
    } else {
        //parsed file  format are: one word(wordWeight first words last) in line, empty line is query splitter.
        bool startQuery = true;
        float wordWeight = 0.0;
        TVector<TString> lemmas;
        lemmas.reserve(10);

        while (pornoConfigFile.ReadLine(line)) {
            if (startQuery) {
                wordWeight = FromString<float>(line);
                startQuery = false;
                continue;
            }

            // empty line - new qquery block
            if (!line) {
                result->AddWordsSequence2Detect(lemmas, wordWeight, collector);
                startQuery = true;
                wordWeight = 0.0;
                lemmas.clear();
                continue;
            }

            lemmas.push_back(line);
        }
    }

    return result.Release();
}

void GetRelevZoneMultipliers(enum RelevLevel zone, float& multiplier1, float& multiplier2) {
    multiplier1 = 1.0f; // for MID_RELEV and BEST_RELEV
    multiplier2 = 0.0f;

    if (zone == HIGH_RELEV)
        multiplier1 = 3.5f;
    else if (zone == BEST_RELEV)
        multiplier2 = 0.2f;
}

