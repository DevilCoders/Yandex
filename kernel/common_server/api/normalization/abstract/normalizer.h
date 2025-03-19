#pragma once
#include <util/generic/string.h>
#include <library/cpp/object_factory/object_factory.h>
#include <library/cpp/json/writer/json_value.h>
#include <kernel/common_server/library/interfaces/container.h>
#include <kernel/common_server/library/scheme/scheme.h>

class IBaseServer;

namespace NCS {
    namespace NNormalizer {
        class IStringNormalizer {
        protected:
            virtual bool DoNormalize(const TStringBuf sbValue, TString& result) const = 0;
        public:
            using TPtr = TAtomicSharedPtr<IStringNormalizer>;
            using TFactory = NObjectFactory::TObjectFactory<IStringNormalizer, TString>;
            virtual ~IStringNormalizer() = default;
            bool Normalize(const TStringBuf sbValue, TString& result) const {
                auto gLogging = TFLRecords::StartContext()("normalizer", GetClassName())("base_value", sbValue);
                if (!DoNormalize(sbValue, result)) {
                    TFLEventLog::Error("cannot normalize value");
                    return false;
                }
                return true;
            }
            Y_WARN_UNUSED_RESULT virtual bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo) = 0;
            virtual NJson::TJsonValue SerializeToJson() const = 0;
            virtual TString GetClassName() const = 0;
            virtual NCS::NScheme::TScheme GetScheme(const IBaseServer& server) const = 0;
        };

        class TStringNormalizerContainer: public TBaseInterfaceContainer<IStringNormalizer> {
        public:
            bool Normalize(const TStringBuf sbValue, TString& result) const {
                if (!Object) {
                    TFLEventLog::Error("no normalizer in container");
                    return false;
                }
                return Object->Normalize(sbValue, result);
            }
        };
    }
}
