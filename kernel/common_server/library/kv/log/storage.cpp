#include "storage.h"
#include <kernel/common_server/library/logging/events.h>

namespace NCS {
    namespace NKVStorage {
        bool TLogStorage::DoPutData(const TString& path, const TBlob& data, const TWriteOptions& /*options*/) const {
            TFLEventLog::Notice("put data")("key", path)("data", Base64Encode(TStringBuf(data.AsCharPtr(), data.size())));
            if (MemoryUsage) {
                TGuard<TMutex> g(Mutex);
                MemoryData[path] = data;
            }
            return true;
        }

        bool TLogStorage::DoGetData(const TString& path, TMaybe<TBlob>& data, const TReadOptions& /*options*/) const {
            if (MemoryUsage) {
                TGuard<TMutex> g(Mutex);
                auto it = MemoryData.find(path);
                if (it == MemoryData.end()) {
                    data = Nothing();
                } else {
                    data = it->second;
                }
                return true;
            } else {
                TFLEventLog::Error("not implemented method for storage");
                return false;
            }
        }

    }
}
