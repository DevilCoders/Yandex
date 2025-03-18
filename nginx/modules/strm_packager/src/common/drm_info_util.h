#pragma once

#include <nginx/modules/strm_packager/src/base/workers.h>

namespace NStrm::NPackager {
    // TODO: use flatbuffers structure and cache drm info
    struct TDrmInfo {
        struct TPssh {
            TString SystemId;
            TString Data;
        };

        TMaybe<TString> IV;
        TString Key;
        TString KeyId;

        TVector<TPssh> Pssh;
    };

    struct TCommonDrmInfo {
        bool Enabled;
        bool WholeSegmentAes128;
        bool AllowDrmContentUnencrypted;
        NThreading::TFuture<TMaybe<TDrmInfo>> Info4Muxer;  // for normal drm
        NThreading::TFuture<TMaybe<TDrmInfo>> Info4Sender; // in case aes128
    };

    TCommonDrmInfo GetCommonDrmInfo(TRequestWorker& request, const ui64 segmentIndex);
}
