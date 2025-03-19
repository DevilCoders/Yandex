#pragma once
#include <kernel/common_server/api/snapshots/fetching/abstract/context.h>

namespace NCS {
    class TFakeSnapshotFetcherContext: public ISnapshotContentFetcherContext {
    private:
        CS_ACCESS(TFakeSnapshotFetcherContext, ui32, ReadyObjects, 0);
        static TFactory::TRegistrator<TFakeSnapshotFetcherContext> Registrator;
    protected:
        virtual NJson::TJsonValue DoSerializeToJson() const override;
        virtual bool DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) override;
    public:
        static TString GetTypeName() {
            return "fake_fetcher";
        }

        virtual TString GetClassName() const override {
            return GetTypeName();
        }
    };
}
