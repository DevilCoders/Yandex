#include "tskv.h"

namespace NCS {
    namespace NLogging {
        void TTSKVStreamRec::Add(const TString& key, const NJson::TJsonValue& value) {
            Buffer.Add(key, value.GetStringRobust());
        }

        void TTSKVStreamRec::Add(const TString& key, const NJson::TMapBuilder& value) {
            Buffer.Add(key, value.GetJson().GetStringRobust());
        }
    }
}
