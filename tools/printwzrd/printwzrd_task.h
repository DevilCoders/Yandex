#pragma once
#include "lib/printer.h"
#include <search/fields/fields.h>
#include <library/cpp/eventlog/eventlog.h>
#include <library/cpp/threading/serial_postprocess_queue/serial_postprocess_queue.h>
#include <util/generic/ptr.h>
#include <util/stream/str.h>

class IRequestWizard;
struct TPrintwzrdOptions;
struct TRequestsInfo;
class IWizardResultsPrinter;

class TPrintwzrdTask : public TSerialPostProcessQueue::IProcessObject {
public:
    TPrintwzrdTask(const IRequestWizard* const wizard, const TString& req, const TPrintwzrdOptions& options, IOutputStream& out, TSelfFlushLogFramePtr frame);
    void ParallelProcess(void*) override;
    void SerialProcess() override;

    const TRequestsInfo* GetInfo() const;
    const TString& GetMessage() const { return Message.Str(); }
    void SetMessage(const TString& message) { Message.Str() = message; }

    static void PrintTotals(IOutputStream& out) {
        Totals.Print(out);
    }

private:
    const IRequestWizard* const Wizard;
    const TPrintwzrdOptions& Options;
    TStringStream Message;
    THolder<IWizardResultsPrinter> Printer;
    TString Request;
    static TRequestsInfo Totals;
    IOutputStream& Out;
    TSelfFlushLogFramePtr Frame;
};
