#pragma once

namespace NSnippets {
    extern const double INVALID_SNIP_WEIGHT;

    enum EStaticAnnotationMode {
        SAM_DISABLED = 0,
        SAM_HIDE_EMPTY = (1 << 1),
        SAM_EXTENDED_BY_LINK = (1 << 2),
        SAM_CAN_USE_CONTENT = (1 << 3),
        SAM_CAN_USE_REFERAT = (1 << 4),
        SAM_DOC_START = (1 << 5),
    };
    typedef int TStatAnnotMode;

    enum ESentsSourceType
    {
        SST_TEXT = 0,
        SST_META_DESCR,
        SST_YACA,
        SST_DMOZ,
        SST_YACA_WITH_TITLE,
        SST_DMOZ_WITH_TITLE,
        SST_TABLE_CELL
    };
    enum EOriginId {
        META_ORIGIN_SENT_ID = -1
    };
}
