#include <nginx/modules/strm_packager/src/common/muxer.h>

#include <util/generic/overloaded.h>

namespace NStrm::NPackager {
    TString IMuxer::GetContentType(const TVector<TTrackInfo const*>& tracksInfo) const {
        bool video = false;
        bool audio = false;
        bool subtitle = false;

        for (TTrackInfo const* const ti : tracksInfo) {
            std::visit(
                TOverloaded{
                    [&video](const TTrackInfo::TVideoParams&) {
                        video = true;
                    },
                    [&audio](const TTrackInfo::TAudioParams&) {
                        audio = true;
                    },
                    [](const TTrackInfo::TTimedMetaId3Params&) {
                        // nothing?
                    },
                    [&subtitle](const TTrackInfo::TSubtitleParams&) {
                        subtitle = true;
                    }},
                ti->Params);
        }

        if (video) {
            return "video";
        }

        if (audio) {
            return "audio";
        }

        if (subtitle) {
            return "text";
        }

        Y_ENSURE(false, "cant make content type");
    }
}
