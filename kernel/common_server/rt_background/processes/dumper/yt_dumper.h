#pragma once

#include "dumper.h"

namespace NCS {

    class TYTDumper: public IRTDumper {
        static TFactory::TRegistrator<TYTDumper> Registrator;

    public:
        virtual NFrontend::TScheme GetScheme(const IBaseServer& server) const override;
        Y_WARN_UNUSED_RESULT virtual bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo) override;
        virtual NJson::TJsonValue SerializeToJson() const override;
        void SerializeToJson(NJson::TJsonValue&) const;

        virtual TString GetClassName() const override {
            return GetTypeName();
        }
        static TString GetTypeName() {
            return "yt_dumper";
        }

    protected:
        virtual bool DoProcessRecords(const TRecordsSetWT& records, const ITableFieldViewer& tableViewer,
                                      const NYT::TTableSchema& tableYtSchema, const bool dryRun) const override;

    private:
        CSA_DEFAULT(TYTDumper, TString, YTCluster);
        CSA_DEFAULT(TYTDumper, TString, YTDir);
        CSA_FLAG(TYTDumper, OptimizeForScan, false);
    };

} // namespace NCS
