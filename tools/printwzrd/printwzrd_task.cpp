#include "printwzrd_task.h"
#include "process_print.h"
#include "lib/options.h"
#include "lib/printer.h"
#include <search/wizard/face/reqwizardface.h>

TRequestsInfo TPrintwzrdTask::Totals;

TPrintwzrdTask::TPrintwzrdTask(const IRequestWizard* const wizard, const TString& req, const TPrintwzrdOptions& options, IOutputStream& out, TSelfFlushLogFramePtr frame)
    : Wizard(wizard)
    , Options(options)
    , Printer(GetResultsPrinter(options, Message))
    , Request(req)
    , Out(out)
    , Frame(frame)
{
    Y_ASSERT(Wizard);
}

void TPrintwzrdTask::ParallelProcess(void*) {
    if (Message.empty()) {
        try {
             NPrintWzrd::ProcessWizardRequest(Wizard, Request, Options, Frame, Printer.Get());
        } catch (const yexception& e) {
            if (Options.PrintExtraOutput) {
                NPrintWzrd::PrintFailInfo(Message, Request, Options.IsTestRun);
            }
        }
    }
}

void TPrintwzrdTask::SerialProcess() {
    Out << GetMessage();
    Out.Flush();

    if (Options.PrintLemmaCount)
        if (const TRequestsInfo* info = GetInfo())
            Totals.Update(*info);
}

const TRequestsInfo* TPrintwzrdTask::GetInfo() const {
    return Printer->GetInfo();
}
