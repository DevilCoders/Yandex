#include "checker.h"
#include <kernel/common_server/library/logging/events.h>

namespace NCS {
    bool TVariableNameChecker::DefaultObjectId(const TStringBuf objectId, const TSet<char>& additionalChars) {
        if (objectId.empty()) {
            TFLEventLog::Error("empty object id");
            return false;
        }
        auto gLogging = TFLRecords::StartContext()("object_id", objectId);
        if (objectId[0] >= '0' && objectId[0] <= '9') {
            TFLEventLog::Error("first char have not be digit");
            return false;
        }
        if (objectId[0] == '_') {
            TFLEventLog::Error("first char have not be _");
            return false;
        }
        if (additionalChars.contains(objectId[0])) {
            TFLEventLog::Error("first char have to be default character (0-9, a-z, A-Z)");
            return false;
        }

        TSet<char> additionalAvailable;
        for (auto&& i : objectId) {
            if (!((i <= '9' && i >= '0') || (i >= 'a' && i <= 'z') || (i >= 'A' && i <= 'Z') || i == '_')) {
                if (additionalChars.contains(i)) {
                    continue;
                }
                TFLEventLog::Error("incorrect char in object_id (only a-z, A-Z, _, 0-9)");
                return false;
            }
        }
        return true;
    }

}
