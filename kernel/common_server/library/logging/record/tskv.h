#pragma once
#include <library/cpp/json/writer/json_value.h>
#include <kernel/common_server/util/logging/tskv_log.h>
#include <kernel/common_server/library/json/builder.h>

namespace NCS {
    namespace NLogging {
        class TTSKVStreamRec {
            NUtil::TTSKVRecord Buffer;
        public:

            template <class T>
            void Add(const TString& key, const T& value) {
                Buffer.Add(key, value);
            }

            void Add(const TString& key, const NJson::TMapBuilder& value);
            void Add(const TString& key, const NJson::TJsonValue& value);

            inline TString ToString() const {
                return Buffer.ToString();
            }
        };
    }
}
