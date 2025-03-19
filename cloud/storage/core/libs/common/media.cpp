#include "media.h"

namespace NCloud {

////////////////////////////////////////////////////////////////////////////////

bool IsDiskRegistryMediaKind(NProto::EStorageMediaKind mediaKind)
{
    switch (mediaKind) {
        case NProto::STORAGE_MEDIA_SSD_NONREPLICATED:
        case NProto::STORAGE_MEDIA_SSD_MIRROR2:
        case NProto::STORAGE_MEDIA_SSD_MIRROR3:
        case NProto::STORAGE_MEDIA_SSD_LOCAL:
            return true;
        default:
            return false;
    }
}

TString MediaKindToString(NProto::EStorageMediaKind mediaKind)
{
    switch (mediaKind) {
        case NProto::STORAGE_MEDIA_HDD:
        case NProto::STORAGE_MEDIA_DEFAULT:
            return "hdd";
        case NProto::STORAGE_MEDIA_HYBRID:
            return "hybrid";
        case NProto::STORAGE_MEDIA_SSD:
            return "ssd";
        case NProto::STORAGE_MEDIA_SSD_NONREPLICATED:
            return "ssd_nonrepl";
        case NProto::STORAGE_MEDIA_SSD_MIRROR2:
            return "ssd_mirror2";
        case NProto::STORAGE_MEDIA_SSD_MIRROR3:
            return "ssd_mirror3";
        case NProto::STORAGE_MEDIA_SSD_LOCAL:
            return "ssd_local";
        default:
            return "unknown";
    }
}

TString MediaKindToComputeType(NProto::EStorageMediaKind mediaKind)
{
    switch (mediaKind) {
        case NProto::STORAGE_MEDIA_HDD:
        case NProto::STORAGE_MEDIA_DEFAULT:
        case NProto::STORAGE_MEDIA_HYBRID:
            return "network-hdd";
        case NProto::STORAGE_MEDIA_SSD:
            return "network-ssd";
        case NProto::STORAGE_MEDIA_SSD_NONREPLICATED:
            return "network-ssd-nonreplicated";
        case NProto::STORAGE_MEDIA_SSD_MIRROR2:
            return "network-ssd-mirror2";
        case NProto::STORAGE_MEDIA_SSD_MIRROR3:
            return "network-ssd-mirror3";
        default:
            return "unknown";
    }
}

bool ParseMediaKind(const TStringBuf s, NProto::EStorageMediaKind* mediaKind)
{
    if (s == "ssd") {
        *mediaKind = NProto::STORAGE_MEDIA_SSD;
    } else if (s == "hybrid") {
        *mediaKind = NProto::STORAGE_MEDIA_HYBRID;
    } else if (s == "hdd") {
        *mediaKind = NProto::STORAGE_MEDIA_HDD;
    } else if (s == "nonreplicated" || s == "ssd_nonrepl") {
        *mediaKind = NProto::STORAGE_MEDIA_SSD_NONREPLICATED;
    } else if (s == "mirror2" || s == "ssd_mirror2") {
        *mediaKind = NProto::STORAGE_MEDIA_SSD_MIRROR2;
    } else if (s == "mirror3" || s == "ssd_mirror3") {
        *mediaKind = NProto::STORAGE_MEDIA_SSD_MIRROR3;
    } else if (s == "local" || s == "ssd_local") {
        *mediaKind = NProto::STORAGE_MEDIA_SSD_LOCAL;
    } else {
        return false;
    }

    return true;
}

}   // namespace NCloud
