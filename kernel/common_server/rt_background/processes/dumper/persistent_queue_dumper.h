#pragma once

#include "dumper.h"

namespace NCS {

    class TPQDumper: public IRTDumper {
        static TFactory::TRegistrator<TPQDumper> Registrator;

    public:
        virtual NFrontend::TScheme GetScheme(const IBaseServer& server) const override;
        Y_WARN_UNUSED_RESULT virtual bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo) override;
        virtual NJson::TJsonValue SerializeToJson() const override;
        void SerializeToJson(NJson::TJsonValue&) const;

        virtual TString GetClassName() const override {
            return GetTypeName();
        }
        static TString GetTypeName() {
            return "persistent_queue_dumper";
        }

    protected:
        virtual bool DoProcessRecords(const TRecordsSetWT& records, const ITableFieldViewer& tableViewer,
                                      const NYT::TTableSchema& tableYtSchema, const bool dryRun) const override;

    private:
        CSA_DEFAULT(TPQDumper, TString, ServiceID);
        CSA_DEFAULT(TPQDumper, TString, MessageIDField);
        CS_ACCESS(TPQDumper, ui64, BufferSize, 500);
    };

    constexpr size_t kMessageSizeLimitInBytes = 5 * 1024 * 1024;

} // namespace NCS
