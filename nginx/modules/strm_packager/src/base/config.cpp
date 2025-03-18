#include <nginx/modules/strm_packager/src/base/config.h>

namespace NStrm::NPackager {
    template <typename T>
    inline void Merge(TMaybe<T> TLocationConfig::*param, TLocationConfig& child, const TLocationConfig parent) {
        if ((child.*param).Empty() && (parent.*param).Defined()) {
            (child.*param).ConstructInPlace(*(parent.*param));
        }
    }

    template <typename T>
    inline void Merge(T* TLocationConfig::*param, TLocationConfig& child, const TLocationConfig parent) {
        if (!(child.*param) && (parent.*param)) {
            child.*param = parent.*param;
        }
    }

    void TLocationConfig::MergeFrom(const TLocationConfig& parent) {
        Merge(&TLocationConfig::PackagerHandler, *this, parent);
        Merge(&TLocationConfig::WorkerChecker, *this, parent);

        Merge(&TLocationConfig::TestURI, *this, parent);
        Merge(&TLocationConfig::ChunkDuration, *this, parent);
        Merge(&TLocationConfig::TestSubrequestsCount, *this, parent);
        Merge(&TLocationConfig::TestShmCacheZone, *this, parent);
        Merge(&TLocationConfig::MaxMediaTsGap, *this, parent);
        Merge(&TLocationConfig::MoovScanBlockSize, *this, parent);
        Merge(&TLocationConfig::MoovShmCacheZone, *this, parent);
        Merge(&TLocationConfig::DescriptionShmCacheZone, *this, parent);
        Merge(&TLocationConfig::MaxMediaDataSubrequestSize, *this, parent);
        Merge(&TLocationConfig::URI, *this, parent);
        Merge(&TLocationConfig::MetaLocation, *this, parent);
        Merge(&TLocationConfig::ContentLocation, *this, parent);
        Merge(&TLocationConfig::SubrequestHeadersDrop, *this, parent);
        Merge(&TLocationConfig::SubrequestClearVariables, *this, parent);
        Merge(&TLocationConfig::SignSecret, *this, parent);
        Merge(&TLocationConfig::EncryptSecret, *this, parent);
        Merge(&TLocationConfig::TranscodersLiveLocationCacheFollow, *this, parent);
        Merge(&TLocationConfig::TranscodersLiveLocationCacheLock, *this, parent);
        Merge(&TLocationConfig::LiveVideoTrackName, *this, parent);
        Merge(&TLocationConfig::LiveAudioTrackName, *this, parent);
        Merge(&TLocationConfig::LiveCmafFlag, *this, parent);
        Merge(&TLocationConfig::LiveFutureChunkLimit, *this, parent);
        Merge(&TLocationConfig::LiveManagerAccess, *this, parent);
        Merge(&TLocationConfig::LiveManager, *this, parent);
        Merge(&TLocationConfig::DrmEnable, *this, parent);
        Merge(&TLocationConfig::DrmRequestUri, *this, parent);
        Merge(&TLocationConfig::DrmMp4ProtectionScheme, *this, parent);
        Merge(&TLocationConfig::DrmWholeSegmentAes128, *this, parent);
        Merge(&TLocationConfig::AllowDrmContentUnencrypted, *this, parent);
        Merge(&TLocationConfig::Prerolls, *this, parent);
        Merge(&TLocationConfig::PrerollsClipId, *this, parent);
        Merge(&TLocationConfig::PlaylistJsonUri, *this, parent);
        Merge(&TLocationConfig::PlaylistJsonArgs, *this, parent);
        Merge(&TLocationConfig::SubtitlesTTMLStyle, *this, parent);
        Merge(&TLocationConfig::SubtitlesTTMLRegion, *this, parent);
    }

    void TLocationConfig::Check() const {
        if (SubrequestHeadersDrop.Defined()) {
            for (const TStringBuf& s : *SubrequestHeadersDrop) {
                for (const char c : s) {
                    Y_ENSURE(tolower(c) == c, "SubrequestHeadersDrop must contain lowercase string (" + TString(s) + ")");
                }
            }
        }
    }

}
