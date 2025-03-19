#include "simpledate.h"

namespace NCS {
    TSimpleDate::TSimpleDate(TInstant instant) {
        sscanf(instant.FormatGmTime("%Y-%m-%d").c_str(), "%" SCNu16 "-%" SCNu8 "-%" SCNu8, &Year, &Month, &Day);
    }

    TString TSimpleDate::ToIso8601() const {
        return Sprintf("%04d-%02d-%02d", Year, Month, Day);
    }

    TString TSimpleDate::ToDMY() const {
        return Sprintf("%02d.%02d.%04d", Day, Month, Year);
    }

    TString TSimpleDate::GetStringHashDataInternal() const {
        TStringBuilder sb;
        sb << ::ToString(Day) << "." << ::ToString(Month) << "." << ::ToString(Year);
        return sb;
    }

    TString TSimpleDate::ToString() const {
        return GetStringHashDataInternal();
    }

    bool operator==(const TSimpleDate& l, const TSimpleDate& r) {
        return std::tie(l.GetYear(), l.GetMonth(), l.GetDay()) == std::tie(r.GetYear(), r.GetMonth(), r.GetDay());
    }

    bool operator<(const TSimpleDate& l, const TSimpleDate& r) {
        return std::tie(l.GetYear(), l.GetMonth(), l.GetDay()) < std::tie(r.GetYear(), r.GetMonth(), r.GetDay());
    }

    bool operator>(const TSimpleDate& l, const TSimpleDate& r) {
        return r < l;
    }

    NFrontend::TScheme TSimpleDate::GetScheme(const IBaseServer& /*server*/) {
        NFrontend::TScheme result;
        result.Add<TFSNumeric>("year").SetMin(1900).SetMax(::Now().Days() / 365 + 1970);
        result.Add<TFSNumeric>("month").SetMin(1).SetMax(12);
        result.Add<TFSNumeric>("day").SetMin(1).SetMax(31);
        return result;
    }
    bool TSimpleDate::SimpleValidation() const {
        if (Month > 12 || Month < 1) {
            TFLEventLog::Log("month have to be in [1, 12]");
            return false;
        }
        if (Day > 31 || Day < 1) {
            TFLEventLog::Log("day have to be in [1, 31]");
            return false;
        }
        return true;
    }
} // namespace NCS
