#pragma once

#include <nginx/modules/strm_packager/src/base/live_manager.h>
#include <nginx/modules/strm_packager/src/base/shm_cache.h>
#include <nginx/modules/strm_packager/src/base/types.h>
#include <nginx/modules/strm_packager/src/common/muxer_mp4.h>
#include <nginx/modules/strm_packager/src/common/timestamps.h>

#include <util/generic/maybe.h>
#include <util/generic/set.h>
#include <util/generic/string.h>

namespace NStrm::NPackager {
    class TLocationConfig {
    public:
        const TString ConfigFileLine;
        TMaybe<ngx_http_handler_pt> PackagerHandler;
        TMaybe<void (*)(const TLocationConfig&)> WorkerChecker;
        bool Checked = false;

        // >> temp
        TMaybe<TString> TestURI;
        TMaybe<int> TestSubrequestsCount;
        TMaybe<TShmZone<TShmCache>> TestShmCacheZone;
        // temp <<

        TMaybe<Ti64TimeMs> ChunkDuration;
        TMaybe<Ti64TimeMs> MaxMediaTsGap;
        TMaybe<ui64> MoovScanBlockSize;
        TMaybe<TShmZone<TShmCache>> MoovShmCacheZone;
        TMaybe<TShmZone<TShmCache>> DescriptionShmCacheZone;
        TMaybe<ui64> MaxMediaDataSubrequestSize;
        TMaybe<TComplexValue> URI;
        TMaybe<TString> MetaLocation;
        TMaybe<TComplexValue> ContentLocation;
        TMaybe<TSet<TStringBuf>> SubrequestHeadersDrop;
        TMaybe<bool> SubrequestClearVariables;
        TMaybe<TString> TranscodersLiveLocationCacheFollow;
        TMaybe<TString> TranscodersLiveLocationCacheLock;
        TMaybe<TComplexValue> LiveVideoTrackName;
        TMaybe<TComplexValue> LiveAudioTrackName;
        TMaybe<TComplexValue> LiveCmafFlag;
        TMaybe<ui32> LiveFutureChunkLimit;

        TMaybe<bool> LiveManagerAccess;
        TLiveManager* LiveManager = nullptr;

        TMaybe<TString> SignSecret;
        TMaybe<TString> EncryptSecret;

        TMaybe<TComplexValue> DrmEnable;              // vod_drm_enabled kaltura alternative
        TMaybe<TComplexValue> DrmRequestUri;          // concat of vod_drm_upstream_location and vod_drm_request_uri
        TMaybe<TComplexValue> DrmMp4ProtectionScheme; // will be converted to TMuxerMP4::EProtectionScheme
        TMaybe<TComplexValue> DrmWholeSegmentAes128;  // instead of container-specific drm encryption, whole segment is encrypted with AES-128 CBC with padding

        TMaybe<TComplexValue> AllowDrmContentUnencrypted; // if content with drm in description requested with DrmEnable==false, packager return 403, unless AllowDrmContentUnencrypted==true

        TMaybe<TComplexValue> Prerolls;
        TMaybe<TComplexValue> PrerollsClipId;

        TMaybe<TComplexValue> PlaylistJsonUri;
        TMaybe<TComplexValue> PlaylistJsonArgs;

        TMaybe<TString> SubtitlesTTMLStyle;
        TMaybe<TString> SubtitlesTTMLRegion;

        void MergeFrom(const TLocationConfig& parent);

        void Check() const;
    };
}
