#include "address_extractor.h"

#include <util/generic/vector.h>

extern const TString AGREE_FIELDS[];
extern size_t AGREE_FIELDS_SIZE;
extern size_t MIN_GEO_INDEX;


void TAddressExtractor::PrintSpaces(int n) const {
    for(int i = 0; i < n * 2; i++) {
        *OutStreamPtr << " ";
    }
}

void TAddressExtractor::PrintField(NFact::TCompoundFieldValuePtr field, int depth) const {
    PrintSpaces(depth);
    *OutStreamPtr << "Name: " << field->GetName() << Endl;

    PrintSpaces(depth);
    *OutStreamPtr << "Text: " << field->GetText() << Endl;

    PrintSpaces(depth);
    *OutStreamPtr << "Rule: " << field->GetRuleName() << Endl;

    PrintSpaces(depth);
    *OutStreamPtr << "CompoundRule: " << field->GetCompoundRuleName() << Endl;


    const TVector<NGzt::TArticlePtr> articles = field->GetArticles();
    for (TVector<NGzt::TArticlePtr>::const_iterator article = articles.begin(); article != articles.end(); ++article) {
        PrintSpaces(depth);
        *OutStreamPtr << "Article " <<  article->GetTitle() << Endl;
    }

    const THashMultiMap<TStringBuf, NFact::TFieldValuePtr>& values = field->GetValues();
    for (THashMultiMap<TStringBuf, NFact::TFieldValuePtr>::const_iterator value = values.begin(); value != values.end(); ++value) {
        PrintSpaces(depth);
        *OutStreamPtr << "FieldName " <<  value->first << Endl;

        PrintField(value->second, depth + 1);
    }

    const THashMultiMap<TStringBuf, NFact::TCompoundFieldValuePtr>& compoundValues = field->GetCompoundValues();
    for (THashMultiMap<TStringBuf, NFact::TCompoundFieldValuePtr>::const_iterator value = compoundValues.begin(); value != compoundValues.end(); ++value) {
        PrintSpaces(depth);
        *OutStreamPtr << "CompoundFieldName " <<  value->first << Endl;

        PrintField(value->second, depth+1);
    }
}

void TAddressExtractor::PrintField(NFact::TFieldValuePtr field, int depth) const {
    PrintSpaces(depth);
    *OutStreamPtr << "Name: " << field->GetName() << Endl;

    PrintSpaces(depth);
    *OutStreamPtr << "Text: " << field->GetText() << Endl;

    for (TVector<NGzt::TArticlePtr>::const_iterator article = field->GetArticles().begin(); article != field->GetArticles().end(); ++article) {
        PrintSpaces(depth);
        *OutStreamPtr << "Article " <<  article->GetTitle() << Endl;
    }
}

void TAddressExtractor::PrintFact(NFact::TFactPtr fact, const NText::TWordSymbols& words) const {



    const std::pair<size_t, size_t>& pos = fact->GetSrcPos();
    *OutStreamPtr << "String: " << NSymbol::ToString(words.begin() + pos.first, words.begin() + pos.second) << Endl;
    *OutStreamPtr << "Rule " <<  fact->GetRuleName() << Endl;

    const THashMultiMap<TStringBuf, NFact::TFieldValuePtr>& values = fact->GetValues();
    for (THashMultiMap<TStringBuf, NFact::TFieldValuePtr>::const_iterator value = values.begin(); value != values.end(); ++value){
        *OutStreamPtr << "FieldName " <<  value->first << Endl;
        PrintField(value->second, 1);
    }

    const THashMultiMap<TStringBuf, NFact::TCompoundFieldValuePtr>& compoundValues = fact->GetCompoundValues();
    for (THashMultiMap<TStringBuf, NFact::TCompoundFieldValuePtr>::const_iterator value = compoundValues.begin(); value != compoundValues.end(); ++value){
        *OutStreamPtr << "CompoundFieldName " <<  value->first << Endl;
        PrintField(value->second, 1);
    }
}

void TAddressExtractor::operator() (const NToken::TSentenceInfo& sentence, const NText::TWordSymbols& words, const TVector<NFact::TFactPtr>& facts) {
    Y_UNUSED(sentence);

    for (size_t i = 0; i < facts.size(); ++i) {
        std::pair<size_t, size_t> sourcePositions;
        sourcePositions.first = words[facts[i]->GetSrcPos().first]->GetSentencePos().first + PreparedTextLength;
        sourcePositions.second = words[facts[i]->GetSrcPos().second - 1]->GetSentencePos().second + PreparedTextLength;

        CurrentBlockResult.push_back(TMineAddressPtr(new TMineAddress(facts[i], sourcePositions)));

        if (Options.DebugFacts) {
            PrintFact(facts[i], words);
        }

        currentFactCount++;
        if (currentFactCount >= Options.MaxFactNumber) {
            ProcessEnd();
        }
    }
}

// Trying to handle with
// Country:
//    City:
//       Address
//       Address
//    City:
//       Address
//       Address
//    City:
//       Address
//       Address
void TAddressExtractor::CombineWithSectionHeader() {
    for(size_t combineLevel = MIN_GEO_INDEX; combineLevel < AGREE_FIELDS_SIZE; combineLevel++) {
        TMineAddressPtr header;
        for(size_t line = 0; line < DocResult.size(); line++) {
            for(size_t j = 0; j < DocResult[line].size(); j++) {
                TMineAddressPtr candidate = DocResult[line][j];
                //next header
                if (candidate->GetMinFilledIndex() == combineLevel) {
                    header = candidate;
                    continue;
                }

                if (!header) {
                    continue;
                }

                // higher level header
                if (candidate->GetMinFilledIndex() > combineLevel) {
                    header = nullptr;
                    continue;
                }

                //trying to combine
                if (candidate->IsAgreedWithGeoPart(header)) {
                    TVector<TMineAddressPtr> result = candidate->CombineWithGeoPart(header);
                    if (result.size() > 0) {
                        DocResult[line][j] = result[0];
                    }
                }
            }
        }
    }
}



//Find first geo facts (one for every geo level) that can be added to every other fact in document
//And combine them into every fact in order from village to country
void TAddressExtractor::CombineWithSingleGeo() {
    TVector<bool> hasGeoOnLevel(AGREE_FIELDS_SIZE);
    TVector<TMineAddressPtr> singleGeoOnLevel(AGREE_FIELDS_SIZE);
    for(size_t level = 0; level < AGREE_FIELDS_SIZE; level++) {
        hasGeoOnLevel[level] = false;
    }

    for(size_t line = 0; line < DocResult.size(); line++) {
        for(size_t j = 0; j < DocResult[line].size(); j++) {
            TMineAddressPtr candidate = DocResult[line][j];

            //take candidates with high geo level
            bool needed = false;
            for(size_t level = MIN_GEO_INDEX; level < AGREE_FIELDS_SIZE; level++) {
                if (!hasGeoOnLevel[level] && candidate->HasField(AGREE_FIELDS[level])) {
                    needed = true;
                    break;
                }
            }

            //test if candidate is aggree with every fact in text
            if (needed) {
                bool agreed = true;
                for(size_t test_line = 0; test_line < DocResult.size(); test_line++) {
                    for(size_t i = 0; i < DocResult[test_line].size(); i++) {
                        TMineAddressPtr testAddr = DocResult[test_line][i];
                        agreed = testAddr->IsAgreedWithGeoPart(candidate);
                        if (!agreed)
                            break;
                    }
                    if (!agreed)
                        break;
                }

                //store for later combining
                if (agreed) {
                    for(size_t level = MIN_GEO_INDEX; level < AGREE_FIELDS_SIZE; level++) {
                        if (!hasGeoOnLevel[level] && candidate->HasField(AGREE_FIELDS[level])) {
                            hasGeoOnLevel[level] = true;
                            singleGeoOnLevel[level] = candidate;
                        }
                    }
                }
            }
        }
    }

    //combine geo facts in every fact in text and take first variant
    for(size_t level = MIN_GEO_INDEX; level < AGREE_FIELDS_SIZE; level++) {
        if (hasGeoOnLevel[level]) {
            TMineAddressPtr geo = singleGeoOnLevel[level];
            for(size_t line = 0; line < DocResult.size(); line++) {
                for(size_t j = 0; j < DocResult[line].size(); j++) {
                    TVector<TMineAddressPtr> result = DocResult[line][j]->CombineWithGeoPart(geo);
                    if (result.size() > 0) {
                        DocResult[line][j] = result[0];
                    }
                }
            }
        }
    }
}

TVector<TMineAddressPtr> TAddressExtractor::GetFilteredFacts() const {
    TList<TMineAddressPtr> filteredResults;
    for(size_t i = 0; i < DocResult.size(); i++) {
        for(size_t j = 0; j < DocResult[i].size(); j++) {
            TMineAddressPtr fact = DocResult[i][j];

            if (Options.OnlyWithNumbers && !fact->HasStreetNumber())
                continue;
            if (Options.OnlyWithCity && !fact->HasCity())
                continue;

            bool add = true;
            if (Options.RemoveSmaller) {
                for(TList<TMineAddressPtr>::iterator it = filteredResults.begin(); filteredResults.end() != it;) {
                    if (fact->IsSmaller(*it)) {
                        add = false;
                        break;
                    }
                    if ((*it)->IsSmaller(fact)) {
                        it = filteredResults.erase(it);
                        continue;
                    }
                    it++;
                }
            }
            if (add)
                filteredResults.push_back(fact);
        }
    }

    return TVector<TMineAddressPtr>(filteredResults.begin(), filteredResults.end());
}

void TAddressExtractor::PrintResult() const {
    TVector<TMineAddressPtr> result = GetFilteredFacts();
    for(TVector<TMineAddressPtr>::const_iterator it = result.begin(); result.end() != it; it++) {
        *OutStreamPtr << (*it)->FirstVariantAsFullAnswer() << Endl;
    }
}

