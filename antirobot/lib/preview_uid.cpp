#include "preview_uid.h"

namespace NAntiRobot {

EPreviewAgentType GetPreviewAgentType(TStringBuf userAgent) {
    if (userAgent.StartsWith("WhatsApp/")) {
        return EPreviewAgentType::OTHER;
    }
    if (userAgent == "Viber" || userAgent.StartsWith("Viber/")) {
        return EPreviewAgentType::OTHER;
    }
    if (userAgent.StartsWith("Mozilla/5.0 (Windows NT 6.1; WOW64) SkypeUriPreview")) {
        return EPreviewAgentType::OTHER;
    }
    return EPreviewAgentType::UNKNOWN;
}

}
