#include <nginx/modules/strm_packager/src/common/convert_subtitles_raw_to_ttml.h>

#include <nginx/modules/strm_packager/src/base/config.h>
#include <nginx/modules/strm_packager/src/base/workers.h>

#include <util/string/split.h>

namespace NStrm::NPackager {

    class TRaw2TTMLConverter {
    public:
        explicit TRaw2TTMLConverter(TRequestWorker& request);

        TTrackInfo const* operator()(TTrackInfo const* orig) const;

        TSampleData operator()(const TSampleData& data) const;

        static bool NeedConvertion(const TVector<TTrackInfo const*>& tracksInfo);

    private:
        static bool IsRawSubtitle(const TTrackInfo& trackInfo);

        TRequestWorker& Request;
    };

    TRaw2TTMLConverter::TRaw2TTMLConverter(TRequestWorker& request)
        : Request(request)
    {
    }

    inline bool TRaw2TTMLConverter::IsRawSubtitle(const TTrackInfo& trackInfo) {
        using TSubtitleParams = TTrackInfo::TSubtitleParams;

        return std::holds_alternative<TSubtitleParams>(trackInfo.Params) &&
               std::get<TSubtitleParams>(trackInfo.Params).Type == TSubtitleParams::EType::RawText;
    }

    inline bool TRaw2TTMLConverter::NeedConvertion(const TVector<TTrackInfo const*>& tracksInfo) {
        for (TTrackInfo const* const trackInfo : tracksInfo) {
            Y_ENSURE(trackInfo);
            if (IsRawSubtitle(*trackInfo)) {
                return true;
            }
        }
        return false;
    }

    inline TTrackInfo const* TRaw2TTMLConverter::operator()(TTrackInfo const* orig) const {
        using TSubtitleParams = TTrackInfo::TSubtitleParams;

        Y_ENSURE(orig);

        if (IsRawSubtitle(*orig)) {
            TTrackInfo* result = Request.GetPoolUtil<TTrackInfo>().New();
            *result = *orig;
            std::get<TSubtitleParams>(result->Params).Type = TSubtitleParams::EType::TTML;
            return result;
        }

        return orig;
    }

    inline TSampleData TRaw2TTMLConverter::operator()(const TSampleData& data) const {
        if (data.DataParams.Format != TSampleData::TDataParams::EFormat::SubtitleRawText) {
            return data;
        }

        TSampleData result = data;

        result.DataParams.Format = TSampleData::TDataParams::EFormat::SubtitleTTML;

        // no reason to call RequireData for subtitles - for now all subtitles must be available without delay
        Y_ENSURE(data.DataFuture.HasValue());

        TVector<TStringBuf> lines;

        if (data.DataSize > 0) {
            Split(TStringBuf((char const*)data.Data, data.DataSize), "\n\r", lines);
        }

        if (lines.empty()) {
            static const char emptyTTML[] = "<tt xmlns=\"http://www.w3.org/ns/ttml\"/>";
            result.DataSize = sizeof(emptyTTML) - 1;
            result.Data = (ui8 const*)emptyTTML;
        } else {
            TStringBuilder str;
            str << "<tt xmlns=\"http://www.w3.org/ns/ttml\" xmlns:tts=\"http://www.w3.org/ns/ttml#styling\" >"
                   "<head>"
                   "<styling>";
            str << "<style xml:id=\"s0\" " << Request.Config.SubtitlesTTMLStyle.GetOrElse(TString()) << "/>";
            str << "</styling>"
                   "<layout>";
            str << "<region xml:id=\"r0\" " << Request.Config.SubtitlesTTMLRegion.GetOrElse(TString()) << "/>";
            str << "</layout>"
                   "</head>"
                   "<body style=\"s0\" region=\"r0\">"
                   "<div>";
            str << "<p begin=\"" << data.GetPts().MilliSeconds() << "ms\" end=\"" << (data.GetPts() + data.CoarseDuration).MilliSeconds() << "ms\">";

            for (const TStringBuf& line : lines) {
                str << "<span>";
                for (const char& c : line) {
                    // xml has only 5 cahracters to escape:
                    //   &amp;      =  &
                    //   &lt;       =  <
                    //   &gt;       =  >
                    //   &quot;     =  "
                    //   &apos;     =  '
                    // and actually we dont need to ecape " and ' here, and maybe > (but better escape for compatibility)

                    if (c == '&') {
                        str << "&amp;";
                    } else if (c == '<') {
                        str << "&lt;";
                    } else if (c == '>') {
                        str << "&gt;";
                    } else if (c == '\"') {
                        str << "&quot;";
                    } else if (c == '\'') {
                        str << "&apos;";
                    } else {
                        str << c;
                    }
                }
                str << "</span><br/>";
            }
            str << "</p>"
                   "</div>"
                   "</body>"
                   "</tt>";

            result.DataSize = str.length();
            result.Data = Request.HangDataInPool(std::move(str)).begin();
        }

        return result;
    }

    TSourceFuture ConvertSubtitlesRawToTTML(TRequestWorker& request, TSourceFuture source) {
        return TSourceConverter<TRaw2TTMLConverter>::Make<TRequestWorker&>(request, source, request);
    }

}
