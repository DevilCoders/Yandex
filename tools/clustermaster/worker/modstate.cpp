#ifndef _win_
#   include <sys/wait.h>
#endif

#include "modstate.h"

#include "log.h"
#include "state_file.h"

#include <tools/clustermaster/proto/target.pb.h>

#include <library/cpp/getopt/opt.h>

#include <util/memory/segmented_string_pool.h>
#include <util/stream/file.h>
#include <util/system/file.h>
#include <util/system/fs.h>
#include <util/system/sigset.h>

#include <iostream>

IOutputStream& operator<<(IOutputStream& os, const NProto::TTargetExitStatus& status)
{
    os << "FinishedTime=" << status.GetFinishedTime()
       << ", ExitStatus=" << status.GetExitStatus();
    return os;
}

int modstate(int argc, char **argv) {
    TString varDirPath;
    TString targetName;
    int task = -1;
    int count = -1;

    int exitStatus = 0;

    bool doPrint = false;
    bool noSave = false;

    // Parse options
    NLastGetopt::TOpts opts;

    opts.SetFreeArgsMin(0);
    opts.SetFreeArgsMax(20);

    opts.AddLongOption('v', "vardir", "vardir path").RequiredArgument("path").StoreResult(&varDirPath);
    opts.AddLongOption("target", "target name").RequiredArgument("name").StoreResult(&targetName);
    opts.AddLongOption("task", "task number").RequiredArgument("number").StoreResult(&task);
    opts.AddLongOption("count", "number task").RequiredArgument("count").StoreResult(&count);
    opts.AddLongOption('p', "print").NoArgument().StoreValue(&doPrint, true);
    opts.AddLongOption('n', "no-save").NoArgument().StoreValue(&noSave, true);

    // Check options
    NLastGetopt::TOptsParseResult optr(&opts, argc, argv);
    if (varDirPath.empty())
        ythrow yexception() << "vardir not specified";

    if (targetName.empty())
        ythrow yexception() << "target not specified";

    if (task == -1)
        ythrow yexception() << "task number not specified";

    if (count == -1)
        ythrow yexception() << "count of task not specified";

    //XXX: Do something before start

    TVector<char *> argVec;
    TVector<TString> freeArgs = optr.GetFreeArgs();
    for (TVector<TString>::const_iterator i = freeArgs.begin(); i != freeArgs.end(); ++i) {
        argVec.push_back(const_cast<char *>(i->data()));
    }
    argVec.push_back(static_cast<char*>(nullptr));

    pid_t childPid = fork();

    if (childPid == -1) {
        // FIXME: errno & exit status have different semantic.
        exitStatus = LastSystemError();
    } else if (childPid == 0) {
        sigset_t sigmask;
        SigEmptySet(&sigmask);
        SigAddSet(&sigmask, SIGTERM);
        SigAddSet(&sigmask, SIGINT);
        SigProcMask(SIG_UNBLOCK, &sigmask, nullptr);

        return execvp(argVec[0], argVec.data());
    } else {
        while (1) {
            pid_t retCode = waitpid(childPid, &exitStatus, 0);
            if (retCode < 0)
                throw yexception() << "waitpid() failed: " << LastSystemErrorText();

            if (WIFEXITED(exitStatus) || WIFSIGNALED(exitStatus))
                break;
        }
    }

    //XXX: Do something after finish

    // Open state
    TString exitStatusFileName = StateFilePathEx(varDirPath, targetName);
    TFile file(exitStatusFileName, OpenAlways | (noSave ? RdOnly : RdWr));
    file.Flock(LOCK_EX);

    // Load state
    // XXX: handle nonexisting state?
    TUnbufferedFileInput in(file);
    NProto::TTargetExitStatuses statuses;

    if (!statuses.ParseFromArcadiaStream(&in)) {
        //If we can't parse file, so it's big problem with it,
        //and I think it's the best way to do nothing
        ythrow yexception() << "cannot parse state from file";
    }

    //status file was absented or graph configuration was changed
    if ((int)statuses.StatusSize() != count) {
        LOG("Recreating statuses file");
        statuses.ClearStatus();
        for (int nTask = 0; nTask < count; nTask++) {
            NProto::TTargetExitStatus* status = statuses.AddStatus();
            status->SetFinishedTime(-1);
            status->SetExitStatus(-1);
        }
    }
    if (task >= (int)statuses.StatusSize())
        ythrow yexception() << "no task " << task << " in this target (" << (int)statuses.StatusSize() << ")";

    NProto::TTargetExitStatus* status = statuses.MutableStatus(task);

    // Print state
    if (doPrint) {
        LOG("Old status: " << *status);
    }

    // Modify state
    time_t now = time(nullptr);
    status->SetFinishedTime(now);
    status->SetExitStatus(exitStatus);

    if (doPrint) {
        LOG("New status: " << *status);
    }

    if (!noSave) {
        // Save state
        file.Seek(0, sSet);
        TUnbufferedFileOutput out(file);
        if (!statuses.SerializeToArcadiaStream(&out))
            ythrow yexception() << "cannot serialize state to file";

        // Truncate file if older size was larger
        file.Resize(statuses.ByteSize());
        file.Flush();
    }

    return 0;
}
