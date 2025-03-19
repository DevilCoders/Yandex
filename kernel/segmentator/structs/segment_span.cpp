#include "structs.h"
#include "segment_span.h"

namespace NSegm {

const char* GetSegmentName(NSegm::ESegmentType type) {
    switch (type) {
    default:
        return "unknown";
    case STP_NONE:
        return "none";
    case STP_AUX:
        return "aux";
    case STP_CONTENT:
        return "content";
    case STP_FOOTER:
        return "footer";
    case STP_HEADER:
        return "header";
    case STP_LINKS:
        return "links";
    case STP_MENU:
        return "menu";
    case STP_REFERAT:
        return "referat";
    }
}

NSegm::ESegmentType GetSegmentTypeByName(TStringBuf name) {
    if (name == "aux") {
        return STP_AUX;
    } else if (name == "content") {
        return STP_CONTENT;
    } else if (name == "footer") {
        return STP_FOOTER;
    } else if (name == "header") {
        return STP_HEADER;
    } else if (name == "links") {
        return STP_LINKS;
    } else if (name == "menu") {
        return STP_MENU;
    } else if (name == "referat") {
        return STP_REFERAT;
    } else if (name == "none") {
        return STP_NONE;
    }
    ythrow yexception() << "Unknown segment type: " << name;
}

bool In(ui8 type, ESegmentType a, ESegmentType b, ESegmentType c, ESegmentType d, ESegmentType e) {
    return type == a || (b && type == b) || (c && type == c) || (d && type == d) || (e && type == e);
}

}
