#pragma once
#include <tools/printwzrd/lib/printer.h>

#include <mapreduce/yt/interface/client.h>

#include <search/fields/fields.h>
#include <search/idl/events.ev.pb.h>

#include <library/cpp/eventlog/eventlog.h>
#include <library/cpp/threading/serial_postprocess_queue/serial_postprocess_queue.h>

#include <util/generic/ptr.h>
#include <util/stream/str.h>

class IRequestWizard;
struct TPrintwzrdOptions;
class IWizardResultsPrinter;

class TNodeEventLogPrinter: public ILogFrameEventVisitor {
public:
    explicit TNodeEventLogPrinter(NYT::TNode& output)
        : Output(output)
    {
    }
private:
    virtual void Visit(const TEvent& event) {
        auto factory = NEvClass::Factory();
        if (event.Class == factory->ClassByName("ReqWizardProcessedRule")) {
            const NEvClass::TReqWizardProcessedRule* processedRule = event.Get<NEvClass::TReqWizardProcessedRule>();
            Output(processedRule->rule_name(), processedRule->successful());
            if (ts) {
                Output(processedRule->rule_name() + TString("_duration"), event.Timestamp - ts);
            }
        }
        ts = event.Timestamp;
    }
private:
    NYT::TNode& Output;
    i64 ts;
};

class TProcessRowTask : public TSerialPostProcessQueue::IProcessObject {
public:
    TProcessRowTask(
        const IRequestWizard* const wizard,
        const NYT::TNode& RequestRow,
        const TPrintwzrdOptions& options,
        TSelfFlushLogFramePtr frame,
        NYT::TTableWriter<NYT::TNode>* output);
    void ParallelProcess(void*) override;
    void SerialProcess() override;

private:
    const IRequestWizard* const Wizard;
    TSearchFields Fields;
    TStringStream WizardData;
    TSelfFlushLogFramePtr FramePtr;
    THolder<IWizardResultsPrinter> Printer;
    NYT::TNode TaskRow;
    NYT::TTableWriter<NYT::TNode>* Output;
    size_t TableIndex;
};
