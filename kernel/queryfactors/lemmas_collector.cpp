#include "lemmas_collector.h"
#include "detectors.h" // for IWordDetector

#include <kernel/indexer/direct_text/dt.h>
#include <util/generic/string.h>
#include <library/cpp/charset/wide.h> // for WideToChar

void TLemmasCollector::AddLemmaAndDetector(const TString& lemma, IWordDetector* detector) {
    AddLemmaAndDetector(lemma, detector, "");
}

void TLemmasCollector::AddLemmaAndDetector(const TString& lemma, IWordDetector* detector, const TString& exactForm) {
    Dict[lemma].push_back(TLemmaInfoAndDetector(detector, exactForm));
}

void TLemmasCollector::Detect(ui32 position, const char* lemma, const TWtringBuf& tok) {
    TIterator it = Dict.find(lemma);

    if (it != Dict.end()) {                 // find lemma in list
        TDetectorList& detList = it->second;
        for (size_t k = 0; k < detList.size(); ++k) {
            IWordDetector* detector = detList[k].Detector;
            if (detector->GetResult())      // some kind of optimization
                continue;

            if (!!detList[k].ExactForm) {
                // simplification of EXACT form of word, simple token
                // some perturbation for make normal token
                const size_t toklen = tok.size();
                TTempBuf buf(toklen + 1);
                WideToChar(tok.data(), toklen + 1, buf.Data(), CODES_YANDEX);
                const char* token = buf.Data();

                if (0 == strcmp(token, detList[k].ExactForm.data())) {
                    detector->Detect(position, lemma);
                }
            } else {
                detector->Detect(position, lemma);
            }
        }
    }
}

void TLemmasCollector::Process(const NIndexerCore::TDirectTextData2& directText) {
    for (size_t entryCnt = 0; entryCnt < directText.EntryCount; ++entryCnt) {
        const NIndexerCore::TDirectTextEntry2& entry = directText.Entries[entryCnt];
        if (!entry.Token)
            continue;
        for (ui32 j = 0; j < entry.LemmatizedTokenCount; ++j) {
            const NIndexerCore::TLemmatizedToken& lemToken = entry.LemmatizedToken[j];
            Detect(entry.Posting, lemToken.LemmaText, entry.Token);
        }
    }
}
