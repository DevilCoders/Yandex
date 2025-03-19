#pragma once

#include <kernel/remorph/facts/fact.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NRemorph {
namespace NMeasure {

struct TField {
    TUtf16String Value;
    TUtf16String NormalizedValue;
    std::pair<size_t, size_t> Pos;

    TField() {
    }

    TField(const TUtf16String& val, const std::pair<size_t, size_t>& pos)
        : Value(val)
        , NormalizedValue(val)
        , Pos(pos)
    {
    }
};

struct TMeasureUnit {
    TField Number;
    TField MeasureUnit;
};

struct TMeasureRecord {
    TVector<TMeasureUnit> Units;
    TField Count;
    bool IsRange;

    TMeasureRecord()
        : IsRange(false)
    {
    }
};

TMeasureRecord ExtractMeasure(const NFact::TFact& fact, bool allForms);

struct TNumberRecord {
    TVector<TField> Numbers;
    TField Count;
    bool IsRange;

    TNumberRecord()
        : IsRange(false)
    {
    }
};

TNumberRecord ExtractNumber(const NFact::TFact& fact, bool allForms);

} // NMeasure
} // NRemorph
