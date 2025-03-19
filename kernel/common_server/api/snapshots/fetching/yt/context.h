#pragma once
#include <kernel/common_server/api/snapshots/fetching/abstract/context.h>

namespace NCS {
    class TYTSnapshotFetcherContext: public ISnapshotContentFetcherContext {
    private:
        CS_ACCESS(TYTSnapshotFetcherContext, ui64, StartTableIndex, 0);
        CS_ACCESS(TYTSnapshotFetcherContext, ui64, SnapshotSize, 0);
        CS_ACCESS(TYTSnapshotFetcherContext, ui32, ErrorsCount, 0);
        static TFactory::TRegistrator<TYTSnapshotFetcherContext> Registrator;
    protected:
        virtual NJson::TJsonValue DoSerializeToJson() const override;
        virtual bool DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) override;
    public:
        static TString GetTypeName() {
            return "yt_fetcher";
        }

        virtual TString GetClassName() const override {
            return GetTypeName();
        }
    };
}
