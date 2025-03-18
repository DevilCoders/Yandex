#include "compiler.h"

#include "compile_task.h"
#include "controller.h"

#include <kernel/remorph/common/verbose.h>

namespace NRemorphCompiler {

TCompiler::TCompiler(size_t threads, ETraceLevel verbosity)
    : Queue()
    , Threads(threads)
{
    SetVerbosityLevel(verbosity);
}

bool TCompiler::Run(const TConfig& config, IOutputStream* log, bool throwOnError) {
    if (log) {
        *log << "Compilation started: " << config.Size() << " unit(s) to build." << Endl;
    }

    TCompileTask::TPtrs tasks;

    for (TUnits::const_iterator iUnit = config.GetUnits().begin(); iUnit != config.GetUnits().end(); ++iUnit) {
        const TUnitConfig& unit = **iUnit;
        TCompileTask::TPtr task(new TCompileTask(unit, log));
        tasks.push_back(task);
    }

    TController controller(Queue, tasks, log);
    controller.Process(Threads);

    size_t failed = 0;
    for (TCompileTask::TPtrs::const_iterator iTask = tasks.begin(); iTask != tasks.end(); ++iTask) {
        if (!(*iTask)->GetStatus()) {
            ++failed;
        }
    }

    if (log) {
        *log << "Compilation finished: " << (tasks.size() - failed) << " unit(s) built";
        if (failed) {
            *log << ", " << failed << " failed";
        }
        *log << "." << Endl;

        if (failed) {
            *log << "Errors summary:" << Endl;
            for (TCompileTask::TPtrs::const_iterator iTask = tasks.begin(); iTask != tasks.end(); ++iTask) {
                const TCompileTask& task = **iTask;
                if (!task.GetStatus()) {
                    *log << "Unit: " << task.GetUnit().Output << Endl;
                    *log << "  rules: " << task.GetUnit().GetLoadOptions().FilePath << Endl;
                    *log << "  error: " << task.GetError() << Endl;
                }
            }
        }
    }

    if (throwOnError && failed) {
        for (TCompileTask::TPtrs::const_iterator iTask = tasks.begin(); iTask != tasks.end(); ++iTask) {
            if (!(*iTask)->GetStatus()) {
                throw yexception() << (*iTask)->GetError();
            }
        }
    }

    return failed == 0;
}

} // NRemorphCompiler
