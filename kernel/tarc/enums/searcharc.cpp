#include "searcharc.h"

EArchiveType ArcTypeFromProto(NArcProto::EArchiveType type) {
    switch(type) {
        case NArcProto::AT_FLAT:
            return AT_FLAT;
        case NArcProto::AT_MULTIPART:
            return AT_MULTIPART;
        default:
            Y_FAIL("unknown ArchiveType");
    }
    return AT_FLAT;
}

TArchiveVersion ArcVersionFromProto(NArcProto::EArchiveVersion version) {
    switch (version) {
    case NArcProto::ARCVERSION:
        return ARCVERSION;
    case NArcProto::ARC_COMPRESSED_EXT_INFO:
        return ARC_COMPRESSED_EXT_INFO;
    default:
        Y_FAIL("unknown ArchiveVersion");
    }
    return ARCVERSION;
}
