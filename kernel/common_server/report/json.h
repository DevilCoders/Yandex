#pragma once

#include "abstract.h"

#include <kernel/common_server/library/searchserver/simple/context/replier.h>
#include <kernel/common_server/util/coded_exception.h>
#include <kernel/common_server/obfuscator/obfuscators/abstract.h>

#include <library/cpp/json/writer/json_value.h>

class TJsonReport: public IFrontendReportBuilder {
private:
    using TBase = IFrontendReportBuilder;

private:
    CS_ACCESS(TJsonReport, bool, SortKeys, false);
    CS_ACCESS(TJsonReport, ui32, LogLineLimit, 1024);
    CSA_DEFAULT(TJsonReport, TString, OverridenContentType);
    CSA_DEFAULT(TJsonReport, NCS::NObfuscator::TObfuscatorContainer, Obfuscator);
    TMap<TString, TString> StrReports;
    TMap<TString, TString> EventCodes;

private:
    NJson::TJsonValue Report = NJson::JSON_NULL;
    TVector<NCS::NLogging::TBaseLogRecord> EventLog;
    TInstant LastEvent = TInstant::Zero();
    TInstant FirstEvent = TInstant::Zero();

    NJson::TJsonValue BuildJsonEventLog() const;
protected:
    virtual void DoAddEvent(TInstant timestamp, const NCS::NLogging::TBaseLogRecord& e) override;
    virtual void DoFinish(const TCodedException& e) override;
    virtual void DoSetError(const TString& info) override;

    bool ShouldSortKeys() const;

public:
    class TGuard: public TBase::TGuard {
    private:
        using TGuardBase = TBase::TGuard;

    public:
        TGuard(IFrontendReportBuilder::TPtr report, int code);

        TJsonReport* operator->() {
            return &JsonReport;
        }

        void AddReportElement(TStringBuf name, NJson::TJsonValue&& elem, const bool allowUndefined = true) {
            JsonReport.AddReportElement(name, std::move(elem), allowUndefined);
        }
        void AddReportElementString(TStringBuf name, TString&& elem) {
            JsonReport.AddReportElementString(name, std::move(elem));
        }

        void SetExternalReport(NJson::TJsonValue&& report) {
            JsonReport.SetExternalReport(std::move(report));
        }
        void SetExternalReportString(TString&& report, const bool flagIsJson = true) {
            JsonReport.SetExternalReportString(std::move(report), flagIsJson);
        }

        TJsonReport& MutableReport() {
            return JsonReport;
        }

        void AddEventCode(const TString& name, const TString& value) {
            JsonReport.EventCodes[name] = value;
        }

    private:
        TJsonReport& JsonReport;
    };

public:
    using IFrontendReportBuilder::IFrontendReportBuilder;

    void AddReportElement(TStringBuf name, NJson::TJsonValue&& elem, const bool allowUndefined = true);
    void AddReportElementString(TStringBuf name, TString&& elem);

    void SetExternalReport(NJson::TJsonValue&& report);
    void SetExternalReportString(TString&& report, const bool flagIsJson = true);

    bool PrintReport(IOutputStream& so) const;

    const NJson::TJsonValue& GetReport() const {
        return Report;
    }
};
