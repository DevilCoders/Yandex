#pragma once
#include <kernel/common_server/util/accessor.h>
#include <kernel/common_server/api/common.h>
#include "config.h"
#include <kernel/common_server/abstract/frontend.h>
#include <kernel/common_server/api/history/manager.h>
#include <kernel/common_server/api/history/cache.h>
#include <kernel/common_server/api/history/db_entities.h>

namespace NCS {
    namespace NLocalization {
        class TResource {
        private:
            class TResourceLocalization {
                RTLINE_ACCEPTOR_DEF(TResourceLocalization, Localization, TString);
                RTLINE_ACCEPTOR_DEF(TResourceLocalization, Value, TString);
            public:

                TResourceLocalization() = default;

                TResourceLocalization(const TString& localizationId, const TString& value)
                    : Localization(localizationId)
                    , Value(value) {

                }

                NJson::TJsonValue SerializeToJson() const;

                Y_WARN_UNUSED_RESULT bool DeserializeFromJson(const NJson::TJsonValue& info);
            };

            RTLINE_ACCEPTOR_DEF(TResource, Id, TString);
            RTLINE_ACCEPTOR(TResource, Revision, ui64, 0);
            RTLINE_ACCEPTOR_DEF(TResource, Localizations, TVector<TResourceLocalization>);
            Y_WARN_UNUSED_RESULT bool DeserializeDataFromJson(const NJson::TJsonValue& info);
            NJson::TJsonValue SerializeDataToJson() const;
        public:
            using TId = TString;
            bool operator!() const {
                return !Id;
            }
            TMaybe<ui64> GetRevisionMaybe() const {
                return Revision;
            }
            const TString& GetInternalId() const {
                return GetId();
            }
            static TString GetTableName() {
                return "cs_localization";
            }
            static TString GetIdFieldName() {
                return "resource_id";
            }
            class TDecoder: public TBaseDecoder {
                RTLINE_ACCEPTOR(TDecoder, Id, i32, -1);
                RTLINE_ACCEPTOR(TDecoder, Meta, i32, -1);
                RTLINE_ACCEPTOR(TDecoder, Revision, i32, -1);

            public:
                TDecoder() = default;
                TDecoder(const TMap<TString, ui32>& decoderBase);
            };

            TResource() = default;
            explicit TResource(const TString& id);

            TMaybe<TString> GetLocalization(const TString& localizationId) const;

            Y_WARN_UNUSED_RESULT bool DeserializeFromJson(const NJson::TJsonValue& info);
            NStorage::TTableRecord SerializeToTableRecord() const;
            NJson::TJsonValue SerializeToJson() const;

            Y_WARN_UNUSED_RESULT bool DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values);
        };

        class TLocalizationHistoryManager: public TIndexedAbstractHistoryManager<TResource> {
        private:
            using TBase = TIndexedAbstractHistoryManager<TResource>;
        public:
            TLocalizationHistoryManager(const IHistoryContext& context, const THistoryConfig& hConfig)
                : TBase(context, "localization_history", hConfig) {

            }
        };

        class TLocalizationDB: public TDBMetaEntitiesManager<TResource>, public ILocalization {
        private:
            using TBase = TDBMetaEntitiesManager<TResource>;
            using TObject = TResource;
        protected:
            virtual TMaybe<TString> GetLocalStringImpl(const TString& localizationId, const TString& resourceId) const override;
        public:

            TLocalizationDB(THolder<IHistoryContext>&& context, const TConfig& config)
                : TBase(std::move(context), config.GetHistoryConfig()) {
                AssertCorrectConfig(TBase::Start(), "cannot start localization");
            }

            ~TLocalizationDB() {
                TBase::Stop();
            }
        };

    }
}
