#include "processrow_task.h"

#include <tools/printwzrd/lib/options.h>
#include <tools/printwzrd/lib/printer.h>

#include <search/begemot/status/result.h>
#include <search/wizard/common/exception.h>
#include <search/wizard/face/reqwizardface.h>

TProcessRowTask::TProcessRowTask(const IRequestWizard* const wizard,
    const NYT::TNode& requestRow,
    const TPrintwzrdOptions& options,
    TSelfFlushLogFramePtr framePtr,
    NYT::TTableWriter<NYT::TNode>* output)
    : Wizard(wizard)
    , WizardData()
    , FramePtr(framePtr)
    , Printer(GetResultsPrinter(options, WizardData))
    , TaskRow(requestRow)
    , Output(output)
    , TableIndex(0)
{
    Y_ASSERT(Wizard);
}

static const TString* GetLastParam(const TCgiParameters& cgi, TStringBuf name) {
    auto range = cgi.equal_range(name);
    return range.first != range.second ? &(--range.second)->second : nullptr;
}

void TProcessRowTask::ParallelProcess(void*) {
    const TString& preparedRequest = TaskRow["prepared_request"].AsString();
    TCgiParameters cgiParams(preparedRequest);
    TSearchFields fields(cgiParams);
    auto handleException = [&preparedRequest](NBg::EResponseResult code, size_t &tableIndex, NYT::TNode& taskRow, const TString& what) {
        if (code == NBg::RR_EMPTY_REQUEST || code == NBg::RR_INVALID_REQUEST) {
            tableIndex = 1;
        } else {
            Cerr << what << "\n srcreq: " << preparedRequest << "";
            tableIndex = 3;
        }
        taskRow["Error"] = TString(what);
    };
    try {
        Wizard->Process(&fields, FramePtr);
    } catch (NBg::TBadRequestError& e) {
        handleException(e.GolovanStatus, TableIndex, TaskRow, e.what());
    } catch (yexception& e) {
        handleException(NBg::RR_INTERNAL_ERROR, TableIndex, TaskRow, e.what());
    }
    if (auto* json = GetLastParam(fields.CgiParam, "wizjson")) {
        TaskRow["data"] = json ? TString::Join("[", *json, "]") : TString("[]");
    } else {
        TAutoPtr<TWizardResults> wizardResults(TWizardResultsCgiPacker::Deserialize(&fields.CgiParam).Release());
        Printer->Print(wizardResults->SourceRequests, wizardResults->RulesResults, fields.CgiParam, "");
        TaskRow["data"] = WizardData.Str();
    }
    if (FramePtr) {
        TaskRow["processed_rules"] = NYT::TNode::CreateMap();
        TNodeEventLogPrinter visitor(TaskRow["processed_rules"]);
        FramePtr->VisitEvents(visitor, NEvClass::Factory());
    }
}

void TProcessRowTask::SerialProcess() {
    Output->AddRow(TaskRow, TableIndex);
}
