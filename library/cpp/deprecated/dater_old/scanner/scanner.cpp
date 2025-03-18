#include <util/charset/unidata.h>

#include "patterns_impl.h"
#include "scanner.h"

namespace NDater {
    namespace NPrivate {
        template <typename TCharType>
        bool HasNumber(const TCharType* p, const TCharType* pe) {
            while (p != pe) {
                if (IsDigit(*p++))
                    return true;
            }

            return false;
        }

        template <typename TCharType>
        TDateCoord FindDate(const TCharType*& p0, const TCharType* pe, TDateCoord (*Pattern)(const TCharType*&, const TCharType*)) {
            const TCharType* p = p0;
            TDateCoord d = Pattern(p, pe);
            if (d) {
                p0 = p;
                return d;
            }
            return TDateCoord();
        }

        template <typename TCharType>
        TDateCoords Scan(const TCharType* p0, const TCharType* pe, bool (*IsSeparator)(TCharType), TDateCoord (*Pattern)(const TCharType*&, const TCharType*)) {
            TDateCoords dates;
            for (const TCharType* p = p0; p < pe; ++p) {
                const TCharType* pp = p;
                TDateCoord d;
                if (p == p0 || IsSeparator(*p)) {
                    d = FindDate(p, pe, Pattern);
                    if (d) {
                        d.Begin += pp - p0;
                        d.End += pp - p0;
                        dates.push_back(d);
                        continue;
                    } else {
                        p = pp;
                    }
                }
            }
            return dates;
        }

        void CutUrl(TStringBuf url, TStringBuf& host, TStringBuf& path) {
            if (url.StartsWith("http://"))
                url.Skip(7);
            else if (url.StartsWith("https://"))
                url.Skip(8);

            url.SplitAt(url.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890.-:"), host, path);
            if (host.find_first_not_of("123456789:.") == TStringBuf::npos)
                host = "";

            host.SplitOff(':');
            path.SplitOff('#');
        }

    }

    TDateCoords ScanUrl(const char* p0, const char* pe) {
        using namespace NPrivate;
        TStringBuf host, path;
        CutUrl(TStringBuf(p0, pe), host, path);

        TDateCoords dates;

        if (!!path) {
            dates = Scan(path.begin(), path.end(), IsUrlSeparator<char>, UrlWordPattern);
            TDateCoords datesdig = Scan(path.begin(), path.end(), IsUrlSeparator<char>, UrlDigPattern);

            dates.insert(dates.end(), datesdig.begin(), datesdig.end());

            FilterInsaneInternetDates(dates);
            FilterOverlappingDates(dates);

            if (dates.empty()) {
                TDateCoords datesxx = Scan(path.begin(), path.end(), IsUrlSeparator<char>, UrlXXPattern);
                dates.insert(dates.end(), datesxx.begin(), datesxx.end());
            }
        }

        FilterInsaneInternetDates(dates);
        FilterOverlappingDates(dates);

        for (TDateCoords::iterator it = dates.begin(); it != dates.end(); ++it) {
            it->Begin += (path.begin() - p0);
            it->End += (path.begin() - p0);
        }

        if (!!host) {
            TDateCoords hdates = ScanHost(host.begin(), host.end());

            for (TDateCoords::iterator it = hdates.begin(); it != hdates.end(); ++it) {
                it->Begin += (host.begin() - p0);
                it->End += (host.begin() - p0);
            }

            dates.insert(dates.begin(), hdates.begin(), hdates.end());
        }

        return dates;
    }

    TDateCoords ScanHost(const char* p0, const char* pe) {
        using namespace NPrivate;

        TDateCoords hdates = Scan(p0, pe, IsUrlSeparator<char>, HostYPattern);

        for (TDateCoords::iterator it = hdates.begin(); it != hdates.end(); ++it)
            it->From = TDaterDate::FromHost;

        FilterOverlappingDates(hdates);

        return hdates;
    }

    TDateCoords ScanText(const wchar16* p0, const wchar16* pe) {
        using namespace NPrivate;
        if (!HasNumber(p0, pe))
            return TDateCoords();
        TDateCoords dates = Scan(p0, pe, IsTextSeparator, TextWordPattern);
        TDateCoords datesXX = Scan(p0, pe, IsTextSeparator, TextXXPattern);
        TDateCoords datesDig = Scan(p0, pe, IsTextDigSeparator, TextDigPattern);
        TDateCoords datesY = Scan(p0, pe, IsTextYSeparator, TextYPattern);
        dates.insert(dates.end(), datesXX.begin(), datesXX.end());
        dates.insert(dates.end(), datesDig.begin(), datesDig.end());
        dates.insert(dates.end(), datesY.begin(), datesY.end());

        FilterOverlappingDates(dates);
        return dates;
    }

    TDateCoords ScanHumanUrl(const wchar16* p0, const wchar16* pe) {
        using namespace NPrivate;
        if (!HasNumber(p0, pe))
            return TDateCoords();
        TDateCoords dates = Scan(p0, pe, IsTextSeparator, TextWordPattern);
        TDateCoords datesXX = Scan(p0, pe, IsTextSeparator, TextXXPattern);
        TDateCoords datesDig = Scan(p0, pe, IsTextSeparator, TextDigPattern);

        TDateCoords datesUrl = Scan(p0, pe, IsUrlSeparator<wchar16>, UrlWordPatternWide);
        TDateCoords datesUrlXX = Scan(p0, pe, IsUrlSeparator<wchar16>, UrlXXPatternWide);
        TDateCoords datesUrlDig = Scan(p0, pe, IsUrlSeparator<wchar16>, UrlDigPatternWide);

        FilterInsaneInternetDates(datesUrlDig);

        dates.insert(dates.end(), datesXX.begin(), datesXX.end());
        dates.insert(dates.end(), datesDig.begin(), datesDig.end());

        dates.insert(dates.end(), datesUrl.begin(), datesUrl.end());
        dates.insert(dates.end(), datesUrlXX.begin(), datesUrlXX.end());
        dates.insert(dates.end(), datesUrlDig.begin(), datesUrlDig.end());

        FilterOverlappingDates(dates);
        return dates;
    }

}

template <>
void Out<NDater::TDateCoord>(IOutputStream& o, const NDater::TDateCoord& p) {
    o << p.ToString();
}
