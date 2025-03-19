#pragma once
#include <kernel/common_server/api/snapshots/fetching/abstract/content.h>

namespace NCS {
    namespace NSnapshots {
        class TRecordsSnapshotFetcher: public ISnapshotContentFetcher {
        private:
            TBlob RawData;
            static TFactory::TRegistrator<TRecordsSnapshotFetcher> Registrator;
            CS_ACCESS(TRecordsSnapshotFetcher, bool, UpsertDiffMode, false);
            CS_ACCESS(TRecordsSnapshotFetcher, TString, Delimiter, ",");
        protected:
            virtual bool DoFetchData(const TDBSnapshot& snapshotInfo, const ISnapshotsController& controller, const IBaseServer& server, const TString& userId) const override;
            virtual NJson::TJsonValue DoSerializeToJson() const override {
                return NJson::JSON_MAP;
            }
            virtual bool DoDeserializeFromJson(const NJson::TJsonValue& /*jsonInfo*/) override {
                return true;
            }
        public:
            void SetRawData(const TBlob data) {
                RawData = data;
            }

            static TString GetTypeName() {
                return "raw_fetcher";
            }

            virtual TString GetClassName() const override {
                return GetTypeName();
            }
        };

    }
}
