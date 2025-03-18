#include "messages.h"

#include "task_list.h"
#include "worker_variables.h"

#include <tools/clustermaster/proto/target.pb.h>

#include <library/cpp/deprecated/split/split_iterator.h>
#include <library/cpp/digest/md5/md5.h>
#include <library/cpp/logger/all.h>

#include <util/folder/dirut.h>
#include <util/random/random.h>
#include <util/stream/file.h>
#include <util/stream/output.h>
#include <util/system/tempfile.h>

#if defined(__FreeBSD__) || defined(_darwin_)

#include <sys/mount.h>
#include <sys/param.h>

#elif defined(_linux_)

#include <sys/vfs.h>

#endif

const NTransgene::TProtoVersion ProtoVersion = 1000;

extern TLog logger;

void FormatFlags(ui32 flags, TStringOutput& r) {
    if (flags & TCommandMessage::CF_RUN)                         r << ToString(TCommandMessage::CF_RUN ) << ' ';
    if (flags & TCommandMessage::CF_FORCE_RUN)                   r << ToString(TCommandMessage::CF_FORCE_RUN) << ' ';
    if (flags & TCommandMessage::CF_FORCE_READY)                 r << ToString(TCommandMessage::CF_FORCE_READY) << ' ';
    if (flags & TCommandMessage::CF_RETRY)                       r << ToString(TCommandMessage::CF_RETRY) << ' ';
    if (flags & TCommandMessage::CF_CANCEL)                      r << ToString(TCommandMessage::CF_CANCEL) << ' ';
    if (flags & TCommandMessage::CF_INVALIDATE)                  r << ToString(TCommandMessage::CF_INVALIDATE) << ' ';
    if (flags & TCommandMessage::CF_MARK_SUCCESS)                r << ToString(TCommandMessage::CF_MARK_SUCCESS) << ' ';
    if (flags & TCommandMessage::CF_RESET_STAT)                  r << ToString(TCommandMessage::CF_RESET_STAT) << ' ';
    if (flags & TCommandMessage::CF_MARK_SKIPPED)                r << ToString(TCommandMessage::CF_MARK_SKIPPED) << ' ';
    if (flags & TCommandMessage::CF_KILL)                        r << ToString(TCommandMessage::CF_KILL) << ' ';
    if (flags & TCommandMessage::CF_REMAIN_SUCCESS)              r << ToString(TCommandMessage::CF_REMAIN_SUCCESS) << ' ';
    if (flags & TCommandMessage::CF_RECURSIVE_UP)                r << ToString(TCommandMessage::CF_RECURSIVE_UP) << ' ';
    if (flags & TCommandMessage::CF_RECURSIVE_DOWN)              r << ToString(TCommandMessage::CF_RECURSIVE_DOWN) << ' ';
}

void FormatState(const TTaskState& state, TStringOutput& r) {
    if (state == TS_IDLE
        || state == TS_PENDING
        || state == TS_RUNNING
        || state == TS_SUCCESS
        || state == TS_FAILED
        || state == TS_DEPFAILED
        || state == TS_CANCELING
        || state == TS_CANCELED
        || state == TS_UNKNOWN
        || state == TS_SKIPPED)
    {
        r << '/' << ToString(state);
    }
}

/*
 * TCommandMessage
 */
TString TCommandMessage::FormatText(const TString& target, int task, ui32 flags, const TTaskState& state) {
    TString result;
    TStringOutput r(result);

    FormatFlags(flags, r);

    if (!target.empty())                        r << target;
    else                                        r << '*';

    if (task != -1)                             r << '/' << task;

    FormatState(state, r);

    return result;
}

/*
 * TCommandMessage2
 */
TString TCommandMessage2::FormatText(const TString& target, const TRepeatedTasks& tasks, ui32 flags) {
    TString result;
    TStringOutput r(result);

    FormatFlags(flags, r);

    if (!target.empty()) {
        r << target;
    } else {
        r << '*';
    }

    if (tasks.size() != 0) {
        r << '/';

        TVector<int> tasksVector(tasks.begin(), tasks.end());
        r << '[' << JoinTaskList(",", tasksVector) << ']';
    }

    return result;
}

/*
 * TFullStatusMessage
 */

/*
 * TSingleStatusMessage
 */

/*
 * ThisStatusMessage
 */

/*
 * TAuthReplyMessage
 */

/*
 * TAuthSuccessMessage
 */

/*
 * TWorkerHelloMessage
 */
TWorkerHelloMessage::TWorkerHelloMessage(ui32 httpPort) {
    SetHttpPort(httpPort);

    unsigned char challenge[AUTH_CHALLENGE_SIZE];
    for (int i = 0; i < AUTH_CHALLENGE_SIZE; ++i)
        challenge[i] = RandomNumber<unsigned char>();

    SetChallenge(reinterpret_cast<const char *>(challenge), AUTH_CHALLENGE_SIZE);
}

void TWorkerHelloMessage::GenerateResponse(TString& output, const TString& keyPath) const {
    MD5 md5;
    unsigned char digest[16];

    if (!keyPath.empty()) {
        TUnbufferedFileInput keyFile(keyPath);
        TString key = keyFile.ReadAll();

        md5.Update(reinterpret_cast<const unsigned char *>(key.data()), key.size());
    }

    md5.Update(reinterpret_cast<const unsigned char *>(GetChallenge().data()), GetChallenge().size());

    md5.Final(digest);

    output = TString(reinterpret_cast<const char *>(digest), 16);
}

/*
 * TPokeMessage
 */
TString TPokeMessage::FormatText(const TString& target, ui32 flags) {
    TString result;
    TStringOutput r(result);

    if (flags & PF_READY)       r << "ready ";

    if (!target.empty())        r << target;
    else                        r << "*";

    r << "/*";

    return result;
}

/*
 * TMultiPokeMessage
 */
TString TMultiPokeMessage::FormatText(const TString& target, const TMultiPokeMessage::TRepeatedTasks& tasks, ui32 flags) {
    TString result;
    TStringOutput r(result);

    if (flags & PF_READY)       r << "ready ";

    if (!target.empty())        r << target;
    else                        r << "*";

    r << "/";

    for (TRepeatedTasks::const_iterator task = tasks.begin(); task != tasks.end(); ++task) {
        if (task != tasks.begin())
            r << ",";
        r << ToString(*task);
    }

    return result;
}

/*
 * TErrorMessage
 */

/*
 * TVariablesMessage
 */
TVariablesMessage::TVariablesMessage(const TWorkerVariables& variables) {
    for (TWorkerVariables::TMapType::const_iterator it = variables.GetMap().begin(); it != variables.GetMap().end(); ++it)
        AddVariable(it->Name, it->Value, VA_SET);
}

void TVariablesMessage::AddVariable(const TString& name, const TString& value, EVariableAction action) {
    NProto::TVariablesMessage::TVariable *variable = NProto::TVariablesMessage::AddVariable();

    variable->SetName(name);
    if (action == VA_SET)
        variable->SetValue(value);
}

TString TVariablesMessage::FormatText() const {
    TString result;
    TStringOutput r(result);

    for (TVariablesMessage::TRepeatedVariable::const_iterator i = GetVariable().begin(); i != GetVariable().end(); ++i) {
        if (i != GetVariable().begin())     r << "; ";

        if (i->HasValue())                  r << "set " << i->GetName() << " \"" << i->GetValue() << "\"";
        else                                r << "unset " << i->GetName();
    }

    return result;
}

/*
 * TDiskspaceMessage
 */
TDiskspaceMessage::TDiskspaceMessage(const char *mountpoint) {
#ifndef _win_
    struct statfs sfs;
    if (statfs(mountpoint, &sfs) == 0) {
        SetTotal(sfs.f_blocks * sfs.f_bsize);
        SetAvail(sfs.f_bavail * sfs.f_bsize);
    } else
#endif
    {
        SetTotal(0);
        SetAvail(0);
    }
}

template <>
void Out<TTaskState>(IOutputStream& out, const TTaskState& s) {
    out << s.GetId() << '/' << s.GetValue();
}

//FIXME! obsoleted
template <>
void Out<NProto::ETaskState>(IOutputStream& out, NProto::ETaskState s) {
    out << (int)s;
}
