#include "structs.h"
#include <util/stream/output.h>
#include <util/string/printf.h>

namespace NDater {
    using namespace NDatetime;

    bool FindField(size_t& pos, size_t& len, const TString& format, const char* code) {
        pos = format.find(code);
        len = strlen(code);
        return TString::npos != pos;
    }

    char* TDaterDate::ToIndexAttr(char* s) const {
        sprintf(s, "%04u%02u%02u", Year, Max<ui32>(Month, 1), Max<ui32>(Day, 1));
        Y_ASSERT(strlen(s) + 1 == INDEX_ATTR_BUFSIZE);
        return s;
    }

    TDaterDate TDaterDate::FromIndexAttr(char* attr) {
        ui32 year, month, day;
        if (sscanf(attr, "%4u%2u%2u", &year, &month, &day) != 3) {
            ythrow yexception() << "Unknown date format in TDaterDate::FromIndexAttr(\"" << attr << "\")";
        }
        return MakeDateFull(year, month, day);
    }

    TString TDaterDate::ToString(TString format, TDaterDate refDate) const {
        size_t pos;
        size_t len;
        if (FindField(pos, len, format, "%d"))
            format.replace(pos, len, Sprintf("%02u", Day));
        if (FindField(pos, len, format, "%m"))
            format.replace(pos, len, Sprintf("%02u", Month));
        if (FindField(pos, len, format, "%Y"))
            format.replace(pos, len, Sprintf("%04u", Year));

        ui32 from = Valid() ? From : (ui32)FromUnknown;
        if (FindField(pos, len, format, "%f"))
            format.replace(pos, len, Sprintf("%s", EncodeFrom(from)));
        if (FindField(pos, len, format, "%F"))
            format.replace(pos, len, Sprintf("%s", EncodeFrom(from, true)));

        if (FindField(pos, len, format, "%p"))
            format.replace(pos, len, EncodePattern(*this, false));
        if (FindField(pos, len, format, "%P"))
            format.replace(pos, len, EncodePattern(*this, true));

        if (FindField(pos, len, format, "%t"))
            format.replace(pos, len, Sprintf("%llu", (unsigned long long)ToTimeT()));
        if (FindField(pos, len, format, "%r"))
            format.replace(pos, len, Sprintf("%u", TrustLevel()));

        if (refDate && Valid()) {
            if (FindField(pos, len, format, "%g"))
                format.replace(pos, len, Sprintf("%d", DiffDays(refDate)));
            if (FindField(pos, len, format, "%G"))
                format.replace(pos, len, Sprintf("%d", DiffMonths(refDate)));
            if (FindField(pos, len, format, "%K"))
                format.replace(pos, len, Sprintf("%d", DiffYears(refDate)));
        }

        return format;
    }

    /*formatted read is not implemented yet*/
    TDaterDate TDaterDate::FromString(const TString& s) {
        if (s.length() < 8)
            return TDaterDate();
        bool relaxed = s.length() == 8 || '@' == s[8];
        ui32 day = 0, month = 0, year = relaxed ? -1 : 0;
        char from = 0;
        char patt = 0;
        sscanf(s.c_str(), "%u/%u/%u@%c.%c", &day, &month, &year, &from, &patt);

        return MakeDate(year, month, day, GetMode(day, month, year), DecodeFrom(from), IsWordPattern(patt),
                        IsLongYear(patt), relaxed);
    }

    TDaterDate TDaterDate::FromSimpleTM(const TSimpleTM& tm, ui16 from) {
        return FromTM(tm.MDay, tm.Mon, tm.Year, from);
    }

    TDaterDate TDaterDate::FromTimeT(time_t t, ui16 from) {
        return FromSimpleTM(TSimpleTM::New(t), from);
    }

    TDaterDate TDaterDate::FromTM(const struct tm& tinfo, ui16 from) {
        return FromSimpleTM(TSimpleTM::New(tinfo), from);
    }

    TDaterDate TDaterDate::FromTM(ui16 tm_mday, ui16 tm_mon, ui16 tm_year, ui16 from) {
        return TDaterDate(ui32(1900 + tm_year), ui32(1 + tm_mon), ui32(tm_mday), (EDateFrom)from);
    }

    TString TDaterDate::EncodePattern(const TDaterDate& d, bool full) {
        if (!d)
            return full ? "unknown" : "?";

        if (!full)
            return d.WordPattern ? d.LongYear ? "W" : "w" : d.LongYear ? "D" : "d";

        TString year = Sprintf("year as %s digits", d.LongYear ? "4" : "1-2");
        return d.Month ? Sprintf("month as %s and ", d.WordPattern ? "word" : "digit") + year : year;
    }

    bool TDaterDate::IsLongYear(char code) {
        return 'W' == code || 'D' == code;
    }

    bool TDaterDate::IsWordPattern(char code) {
        return 'W' == code || 'w' == code;
    }

    const char* TDaterDate::EncodeFrom(ui32 from, bool full) {
        switch (from) {
            case FromExternal:
                return full ? "external sources" : "X";
            case FromUrl:
                return full ? "url" : "U";
            case FromUrlId:
                return full ? "id in url" : "I";
            case FromTitle:
                return full ? "title" : "T";
            case FromContent:
                return full ? "content" : "B";
            case FromText:
                return full ? "general text" : "Y";
            case FromFooter:
                return full ? "footer" : "F";
            case FromHost:
                return full ? "host" : "H";
            case FromBeforeMainContent:
                return full ? "before main content" : "L";
            case FromMainContentStart:
                return full ? "main content start" : "S";
            case FromMainContent:
                return full ? "main content" : "M";
            case FromMainContentEnd:
                return full ? "main content end" : "E";
            case FromAfterMainContent:
                return full ? "after main content" : "N";
            default:
                return full ? "unknown" : "?";
        }
    }

    TDaterDate::EDateFrom TDaterDate::DecodeFrom(char code) {
        switch (code) {
            case 'X':
                return FromExternal;
            case 'U':
                return FromUrl;
            case 'I':
                return FromUrlId;
            case 'T':
                return FromTitle;
            case 'B':
                return FromContent;
            case 'Y':
                return FromText;
            case 'F':
                return FromFooter;
            case 'H':
                return FromHost;
            case 'L':
                return FromBeforeMainContent;
            case 'S':
                return FromMainContentStart;
            case 'M':
                return FromMainContent;
            case 'E':
                return FromMainContentEnd;
            case 'N':
                return FromAfterMainContent;
            default:
                return FromUnknown;
        }
    }
}

template <>
void Out<NDater::TDaterDate>(IOutputStream& o, const NDater::TDaterDate& p) {
    o << p.ToString();
}
