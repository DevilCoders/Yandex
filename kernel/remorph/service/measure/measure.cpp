#include "measure.h"

#include <kernel/gazetteer/gazetteer.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/system/yassert.h>
#include <util/charset/unidata.h>
#include <library/cpp/containers/sorted_vector/sorted_vector.h>

namespace NRemorph {
namespace NMeasure {

using namespace NSorted;

namespace {

const wchar16 ZERO_GROUP[] = {'0', '0', '0', 0};

const wchar16 WZERO = '0';
const wchar16 WMINUS = '-';
const wchar16 WDOT = '.';


const TStringBuf POWER = "power";
const TStringBuf MULTIPLIER = "multiplier";
const TStringBuf MEASURE = "measure";
const TStringBuf MEASURE_DENOM = "measure_denom";
const TStringBuf ZERO = "0";
const TStringBuf WHOLE = "whole";
const TStringBuf FRACT = "fract";
const TStringBuf SIGN = "sign";
const TStringBuf DENOM = "denom";
const TStringBuf NUM = "num";
const TStringBuf COUNT = "count";
const TStringBuf RANGE_MARK = "range_mark";

const TStringBuf GROUPS[] = {TStringBuf("grp12"), TStringBuf("grp9"), TStringBuf("grp6"), TStringBuf("grp3")};


void RemoveNonDigit(TUtf16String& s) {
    //for (size_t i = !s.Empty() && IsDash(s.at(0)) ? 1 : 0; i < s.size();) {
    for (size_t i = 0; i < s.size();) {
        if (::IsDigit(s.at(i))) {
            ++i;
        } else {
            s.remove(i, 1);
        }
    }
}

void TrimTrailingZeros(TUtf16String& s) {
    while (!s.empty() && s.back() == wchar16('0')) {
        s.pop_back();
    }
}

inline void PadGroup(TUtf16String& s) {
    Y_ASSERT(s.length() <= 3);
    if (s.length() < 3) {
        s.prepend(3 - s.length(), wchar16('0'));
    }
}

inline TUtf16String GetTextVal(const NFact::TFieldValueContainer& container, const TStringBuf& field) {
    TVector<NFact::TFieldValuePtr> fval = container.GetValues(field);
    return fval.empty() ? TUtf16String() : fval.front()->GetText();
}

i32 GetPower(const NFact::TFieldValue& field) {
    const TVector<NGzt::TArticlePtr>& articles = field.GetArticles();
    for (size_t i = 0; i < articles.size(); ++i) {
        const NGzt::TFieldDescriptor* f = articles[i].FindField(POWER);
        if (nullptr != f) {
            switch (f->cpp_type()) {
            case NGzt::TFieldDescriptor::CPPTYPE_INT32:
                for (NGzt::TProtoFieldIterator<i32> it(*articles[i], f); it.Ok(); ++it) {
                    return *it;
                }
                break;
            default:
                Y_FAIL("Invalid value type");
                break;
            }
        }
    }
    return 0;
}

i32 GetPower(const NFact::TFieldValueContainer& container, const TStringBuf& field) {
    TVector<NFact::TFieldValuePtr> fval = container.GetValues(field);
    if (fval.empty())
        return 0;

    return GetPower(*fval.front());
}

TUtf16String JoinWholeFract(i32 power, TUtf16String number, TUtf16String fract) {
    if (power > 0) {
        if (size_t(power) >= fract.size()) {
            number.append(fract).append(power - fract.size(), WZERO);
            fract.clear();
        } else {
            number.append(fract, 0, power);
            fract.remove(0, power);
        }
    } else if (power < 0) {
        const size_t negPower = -power;
        if (negPower >= number.size()) {
            fract.prepend(number).prepend(negPower - number.size(), WZERO);
            number.clear();
        } else {
            fract.prepend(number, number.size() - negPower, negPower);
            number.remove(number.size() - negPower, negPower);
        }
    }

    if (!fract.empty()) {
        TrimTrailingZeros(fract);
        if (number.empty()) {
            number.AssignAscii(ZERO);
        }
        number.append('.').append(fract);
    }
    return number;
}

TUtf16String ConvertToNumber(const NFact::TCompoundFieldValue& val, i32 power) {
    power += GetPower(val, MULTIPLIER);

    TUtf16String groupNum;
    for (size_t i = 0; i < Y_ARRAY_SIZE(GROUPS); ++i) {
        TUtf16String grp = GetTextVal(val, GROUPS[i]);
        if (!groupNum.empty() && grp.empty()) {
            groupNum.append(ZERO_GROUP);
        }
        if (!grp.empty()) {
            RemoveNonDigit(grp);
            PadGroup(grp);
            groupNum.append(grp);
        }
    }
    TUtf16String wholeNum = GetTextVal(val, WHOLE);
    RemoveNonDigit(wholeNum);
    if (!groupNum.empty()) {
        PadGroup(wholeNum);
        wholeNum.prepend(groupNum);
    }

    const TUtf16String denomNum = GetTextVal(val, DENOM);
    if (denomNum.empty()) {
        return JoinWholeFract(power, wholeNum, GetTextVal(val, FRACT));
    } else {
        try {
            i64 denomVal = FromString(denomNum);
            if (denomVal != 0L) {
                return ToWtring(double(FromString<i64>(wholeNum) * std::pow(10.0, power)) / double(denomVal));
            }
        } catch (const TFromStringException&) {
        }
    }

    return TUtf16String();
}

const size_t NUM_PART_LENGTH = 18;

// Layout:
// 1) Zero-padded whole part with 19-symbol length (one zero for sign position)
// 2) Dot symbol if fractional part is presented
// 3) Optional fractional part with not more than 18-symbol length
void NormalizeNumber(TUtf16String& num) {
    const size_t dotPos = num.find_first_of(WDOT);
    const size_t wholeLength = TUtf16String::npos == dotPos ? num.length() : dotPos;

    if (TUtf16String::npos != dotPos) {
        const size_t fractLength = num.length() - dotPos - 1;
        if (fractLength > NUM_PART_LENGTH) {
            num.erase(dotPos + 1 + NUM_PART_LENGTH);
        } else if (0 == fractLength) {
            num.erase(dotPos);
        }
    }

    if (wholeLength > NUM_PART_LENGTH) {
        num.remove(0, wholeLength - NUM_PART_LENGTH);
    } else if (wholeLength < NUM_PART_LENGTH) {
        num.prepend(NUM_PART_LENGTH - wholeLength, WZERO);
    }

    num.prepend(WZERO); // Zero at sign position
}

void ExtractNumberValues(TVector<TField>& numFields, const NFact::TCompoundFieldValue& val, i32 power, bool allForms) {
    numFields.clear();
    TUtf16String number = ConvertToNumber(val, power);

    // Can be empty in case of any parsing errors
    if (!number.empty()) {

        numFields.push_back(TField(number, val.GetSrcPos()));
        NormalizeNumber(numFields.back().NormalizedValue);

        // If number has sign then create two records with and without sign
        const TUtf16String sign = GetTextVal(val, SIGN);
        if (sign.empty())
            return;

        if (allForms && sign == TWtringBuf(&WMINUS, 1)) {
            number.prepend(sign);
            numFields.push_back(TField(number, val.GetSrcPos()));
            numFields.back().NormalizedValue = numFields.front().NormalizedValue;
        } else {
            numFields.back().Value.prepend(sign);
        }

        // In normalized value ignore all other signs except "-"
        if (sign == TWtringBuf(&WMINUS, 1)) {
            numFields.back().NormalizedValue.replace(0, 1, &WMINUS, 0, 1);
        }
    }
}

// Each measure unit may have itsown multiplicator. Return it together with measure label, ordered by multiplicator
void ExtractMeasureUnits(const NFact::TFact& fact, TSimpleMap<i32, TField>& measureFields) {
    TVector<NFact::TFieldValuePtr> measure = fact.GetValues(MEASURE);
    if (measure.empty())
        return;

    TVector<NFact::TFieldValuePtr> measureDenom = fact.GetValues(MEASURE_DENOM);

    if (measureDenom.empty()) {
        const std::pair<size_t, size_t> pos = measure.front()->GetSrcPos();
        for (size_t i = 0; i < measure.size(); ++i) {
            measureFields.insert(std::make_pair(GetPower(*measure[i]), TField(measure[i]->GetText(), pos)));
        }
    } else {
        const std::pair<size_t, size_t> pos(measure.front()->GetSrcPos().first, measureDenom.front()->GetSrcPos().second);
        TUtf16String lemma;
        for (size_t i = 0; i < measure.size(); ++i) {
            const i32 mPower = GetPower(*measure[i]);
            for (size_t j = 0; j < measureDenom.size(); ++j) {
                lemma.assign(measure[i]->GetText()).append('/').append(measureDenom[j]->GetText());
                measureFields.insert(std::make_pair(mPower - GetPower(*measureDenom[j]), TField(lemma, pos)));
            }
        }
    }
}

void Put(TMeasureRecord& res, const TVector<TField>& numFields, const TField& measureField) {
    for (TVector<TField>::const_iterator iNum = numFields.begin(); iNum != numFields.end(); ++iNum) {
        res.Units.emplace_back();
        res.Units.back().Number = *iNum;
        res.Units.back().MeasureUnit = measureField;
    }
}

TField ExtractCount(const NFact::TFact& fact) {
    TVector<NFact::TCompoundFieldValuePtr> nums = fact.GetCompoundValues(COUNT);
    if (!nums.empty()) {
        Y_ASSERT(nums.size() == 1);
        TVector<TField> items;
        ExtractNumberValues(items, *nums.front(), 0, false);
        if (!items.empty()) {
            Y_ASSERT(items.size() == 1);
            return items.back();
        }
    }
    return TField();
}

} // unnamed namespace

TMeasureRecord ExtractMeasure(const NFact::TFact& fact, bool allForms) {
    TMeasureRecord res;

    TSimpleMap<i32, TField> measureUnits;
    ExtractMeasureUnits(fact, measureUnits);
    if (measureUnits.empty()) {
        return res;
    }

    const i32 factPower = GetPower(fact, MULTIPLIER);
    TVector<TField> numFields;
    TVector<NFact::TCompoundFieldValuePtr> nums = fact.GetCompoundValues(NUM);
    for (size_t i = 0; i < nums.size(); ++i) {
        i32 measurePower = measureUnits.front().first;
        ExtractNumberValues(numFields, *nums[i], factPower + measurePower, allForms);
        Put(res, numFields, measureUnits.front().second);
        for (TSimpleMap<i32, TField>::const_iterator iMeasure = measureUnits.begin() + 1; iMeasure != measureUnits.end(); ++iMeasure) {
            if (measurePower != iMeasure->first) {
                measurePower = iMeasure->first;
                ExtractNumberValues(numFields, *nums[i], factPower + measurePower, allForms);
            }
            Put(res, numFields, iMeasure->second);
        }
    }
    res.Count = ExtractCount(fact);
    res.IsRange = !fact.GetValues(RANGE_MARK).empty();
    return res;
}

TNumberRecord ExtractNumber(const NFact::TFact& fact, bool allForms) {
    TNumberRecord res;

    if (fact.GetValues(MEASURE))
        return res;

    const i32 factPower = GetPower(fact, MULTIPLIER);
    TVector<TField> numFields;
    TVector<NFact::TCompoundFieldValuePtr> nums = fact.GetCompoundValues(NUM);
    for (size_t i = 0; i < nums.size(); ++i) {
        ExtractNumberValues(numFields, *nums[i], factPower, allForms);
        res.Numbers.insert(res.Numbers.end(), numFields.begin(), numFields.end());
    }
    res.Count = ExtractCount(fact);
    res.IsRange = !fact.GetValues(RANGE_MARK).empty();
    return res;
}

} // NMeasure
} // NRemorph
