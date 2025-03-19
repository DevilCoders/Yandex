#pragma once

#include "scanner.h"
#include "text_concatenator.h"

namespace ND2 {

struct TDate : public NSegm::TSpan {
    ELanguage   Language;

    struct TPatternFeatures {
        ui64 HasTime              : 1;
        ui64 HasDay               : 1;
        ui64 HasMonth             : 1;
        ui64 HasYear              : 1;
        ui64 Has4DigitsInYear     : 1;
        ui64 MonthIsWord          : 1;
        ui64 FromDateRange        : 1;
        ui64 FromAmbiguousPattern : 1;
        ui64 FromUrlId               : 1;
        ui64 FromHost             : 1;
    } Features;

    struct TData {
        ui32 Day   : 5;
        ui32 Month : 4;
        ui32 Year  : 13;
        bool operator<(const TData& rhs) const {
            return Year < rhs.Year ||
                   ((Year == rhs.Year && Month < rhs.Month) ||
                   (Year == rhs.Year && Month == rhs.Month && Day < rhs.Day));
        }

        bool operator>(const TData& rhs) const {
            return rhs < *this;
        }

        bool operator<=(const TData& rhs) const {
            return !(*this > rhs);
        }

        bool operator>=(const TData& rhs) const {
            return !(*this < rhs);
        }

        bool operator==(const TData& rhs) const {
            return Year == rhs.Year && Month == rhs.Month && Day == rhs.Day;
        }

        bool operator!=(const TData& rhs) const {
            return !(Year == rhs.Year && Month == rhs.Month && Day == rhs.Day);
        }

        bool NoYearLess(const TData& rhs) const {
            return Month < rhs.Month || (Month == rhs.Month && Day < rhs.Day);
        }
    } Data;

    TDate()
        : Language(LANG_UNK)
    {
        Zero(Features);
        Zero(Data);
    }

    TDate(const NSegm::TSpan& sp)
        : NSegm::TSpan(sp)
        , Language(LANG_UNK)
    {
        Zero(Features);
        Zero(Data);
    }
};

typedef TVector<TDate> TDates;

enum EDaterMode {
    DM_MAIN_DATES = 0,
    DM_MAIN_DATES_MAIN_RANGES = 1,
    DM_ALL_DATES = 2,
    DM_ALL_DATES_ALL_RANGES = 3,
    DM_ALL_DATES_MAIN_RANGES = 4,
};

class TDaterScanContext {
    NImpl::TNormTextConcatenator Concatenator;
    NImpl::TScanner Scanner;

    TUtf16String CurrentText;
    NSegm::TPosCoords CurrentCoords;
    NImpl::TChunks CurrentChunks;
    NSegm::TRanges CurrentRanges;

public:
    TDaterDocumentContext Document;

    TDates UrlDates;
    TDates TitleDates;
    TDates TextDates;

    bool LookForDigits;
    bool AcceptNoYear;

    TDaterScanContext(bool lookForDigits = true, bool acceptNoYear = false)
        : LookForDigits(lookForDigits)
        , AcceptNoYear(acceptNoYear)
    {
    }

    void Clear() {
        Reset();
        Document.Clear();
    }

    void Reset();
    void Process(EDaterMode mode = DM_ALL_DATES_MAIN_RANGES);

public:
    // for nonstandard usecases
    void PrepareForScan();
    void ScanUrl();
    bool ScanTextRange(NSegm::TRange, TDates& out, bool withdateranges);
protected:
    void PrepareUrl(TWtringBuf in);
    void ResetCurrent();
};

}
