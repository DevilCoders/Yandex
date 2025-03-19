#include "abstract.h"

namespace NBServer {

    TMaybe<TRTBackgroundProcessContainer> IRTBackgroundProcessesStorage::GetObject(const TString& processId) const {
        TMap<TString, TRTBackgroundProcessContainer> objects;
        if (!GetObjects(objects)) {
            TFLEventLog::Log("cannot fetch objects");
            return Nothing();
        }
        auto it = objects.find(processId);
        if (it == objects.end()) {
            return Nothing();
        } else {
            return it->second;
        }
    }

}
