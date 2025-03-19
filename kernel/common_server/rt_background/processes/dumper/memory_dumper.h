#pragma once

#include "dumper.h"

namespace NCS {

    TVector<TString>& GetMemoryDumperStorage();

    class TMemoryDumper: public IRTDumper {
        static TFactory::TRegistrator<TMemoryDumper> Registrator;

    public:
        virtual NFrontend::TScheme GetScheme(const IBaseServer& server) const override;
        Y_WARN_UNUSED_RESULT virtual bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo) override;
        virtual NJson::TJsonValue SerializeToJson() const override;

        virtual TString GetClassName() const override {
            return GetTypeName();
        }
        static TString GetTypeName() {
            return "memory_dumper_for_tests_only";
        }

    protected:
        virtual bool DoProcessRecords(const TRecordsSetWT& records, const ITableFieldViewer& tableViewer,
                                      const NYT::TTableSchema& tableYtSchema, const bool dryRun) const override;
    };

} // namespace NCS
