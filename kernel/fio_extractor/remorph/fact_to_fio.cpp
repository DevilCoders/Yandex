#include "fact_to_fio.h"
#include <library/cpp/charset/wide.h>

using namespace NFact;

namespace {

    bool GetFactValue(const TFieldValueContainer& fact,
                      const TStringBuf& fieldName, TUtf16String& val,
                      std::pair<size_t, size_t>& valueRange)
    {
        const TVector<TFieldValuePtr> values = fact.GetValues(fieldName);
        if(values.empty())
            return false;
        val = values[0]->GetText();
        valueRange = values[0]->GetSrcPos();
        return true;
    }

    size_t GetRangeSize(std::pair<size_t, size_t>& val) {
        return val.second - val.first;
    }

    const TFieldValueContainer* SelectInitials(const TFact& fact) {
        TVector<TCompoundFieldValuePtr> values = fact.GetCompoundValues("initials");
        if (values.empty()) {
            return &fact;
        } else {
            return values[0].Get();
        }
    }

    void ParseInitials(const TFieldValueContainer* initials, TFullFIO& fullFio, TFIOOccurence& fioOcc) {
        TUtf16String wVal;
        std::pair<size_t, size_t> valueRange;

        if (GetFactValue(*initials, "initial_name", wVal, valueRange)) {
            if (GetRangeSize(valueRange) == 1)
            {
                fullFio.Name = WideToChar(wVal, CODES_YANDEX) ;
                fullFio.InitialName = true;
                fioOcc.NameMembers[InitialName] = TWordHomonymNum(valueRange.first,0);
            }
        }

        if (GetFactValue(*initials, "initial_patronymic", wVal, valueRange)) {
            if (GetRangeSize(valueRange) == 1)
            {
                fullFio.Patronymic = WideToChar(wVal, CODES_YANDEX) ;
                fullFio.InitialPatronymic = true;
                fioOcc.NameMembers[InitialPatronymic] = TWordHomonymNum(valueRange.first,0);
            }
        }
    }

}

void NFioExtractor::Fact2FullFio(const TFact& fact, TFullFIO& fullFio, TFIOOccurence& fioOcc) {
    TUtf16String wVal;
    std::pair<size_t, size_t> valueRange;

    if (GetFactValue(fact, "surname", wVal, valueRange)) {
        fullFio.Surname = WideToChar(wVal, CODES_YANDEX);
        if (GetRangeSize(valueRange) == 1) {
            fioOcc.NameMembers[Surname] = TWordHomonymNum(valueRange.first,0);
        }
        else
            fullFio.MultiWordFio = true;
    }

    if (GetFactValue(fact, "first_name", wVal, valueRange)) {
        fullFio.Name = WideToChar(wVal, CODES_YANDEX);
        if (GetRangeSize(valueRange) == 1) {
            fioOcc.NameMembers[FirstName] = TWordHomonymNum(valueRange.first,0);
        }
        else
            fullFio.MultiWordFio = true;
    }

    if (GetFactValue(fact, "patronymic", wVal, valueRange)) {
        fullFio.Patronymic = WideToChar(wVal, CODES_YANDEX);
        if (GetRangeSize(valueRange) == 1) {
            fioOcc.NameMembers[MiddleName] = TWordHomonymNum(valueRange.first,0);
        }
        else
            fullFio.MultiWordFio = true;
    }
    size_t firstIndex = fact.GetSrcPos().first;
    size_t lastIndex = fact.GetSrcPos().second - 1;
    fioOcc.SetPair(firstIndex, lastIndex);

    ParseInitials(SelectInitials(fact), fullFio, fioOcc);
    fullFio.FirstNameFromMorph = !fact.GetValues("first_name_from_morph").empty();
}
