#include "compile_task.h"

#include <kernel/remorph/cascade/cascade.h>
#include <kernel/remorph/common/load_options.h>
#include <kernel/remorph/matcher/matcher.h>
#include <kernel/remorph/proc_base/matcher_base.h>
#include <kernel/remorph/tokenlogic/tlmatcher.h>
#include <kernel/remorph/engine/char/char.h>

#include <util/generic/yexception.h>
#include <util/stream/file.h>

namespace NRemorphCompiler {

TCompileTask::TCompileTask(const TUnitConfig& unit, IOutputStream* log)
    : Unit(unit)
    , Log(log)
    , Status(true)
    , Error()
    , Notifier(nullptr)
{
}

void TCompileTask::SetNotifier(INotifier* notifier) {
    Notifier = notifier;
}

void TCompileTask::ResetError() {
    Status = true;
    Error = "";
}

void TCompileTask::SetError(const TString& error) {
    Status = false;
    Error = error;
}

void TCompileTask::Process(void* ThreadSpecificResource) {
    Y_UNUSED(ThreadSpecificResource);

    if (!Status) {
        *Log << "Skipped: " << Unit.Output << Endl;

        return;
    }

    IOutputStream* output = &Cout;
    THolder<TOFStream> outputFile;

    if (!Unit.Output.empty()) {
        outputFile.Reset(new TOFStream(Unit.Output));
        output = outputFile.Get();
    }

    const NRemorph::TFileLoadOptions& loadOptions = Unit.GetLoadOptions();
    THolder<NMatcher::TMatcherBase> matcher;

    if (Log) {
        *Log << "Started: " << Unit.Output << Endl;
    }

    try {
        switch (Unit.Type) {
        case NMatcher::MT_REMORPH:
            matcher.Reset(NCascade::TCascade<NReMorph::TMatcher>::Load(loadOptions.FilePath, loadOptions.Gazetteer, loadOptions.BaseDirPath).Release());
            break;
        case NMatcher::MT_TOKENLOGIC:
            matcher.Reset(NCascade::TCascade<NTokenLogic::TMatcher>::Load(loadOptions.FilePath, loadOptions.Gazetteer, loadOptions.BaseDirPath).Release());
            break;
        case NMatcher::MT_CHAR:
            matcher.Reset(NCascade::TCascade<NReMorph::TCharEngine>::Load(loadOptions.FilePath, loadOptions.Gazetteer, loadOptions.BaseDirPath).Release());
            break;
        }

        matcher->SaveToStream(*output);
    } catch (const yexception& error) {
        Status = false;
        if (Log) {
            *Log << "Error: " << error.what() << Endl;
        }
        Error = error.what();
    }

    if (Log) {
        *Log << "Finished: " << Unit.Output << Endl;
    }

    if (Notifier) {
        Notifier->Notify();
    }
}

} // NRemorphCompiler
