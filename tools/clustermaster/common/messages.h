#pragma once

#include "target.h"
#include "target_type_parameters.h"

#include <tools/clustermaster/proto/messages.pb.h>

#include <library/cpp/deprecated/transgene/transgene.h>

#include <util/generic/maybe.h>
#include <util/network/sock.h>

#define AUTH_CHALLENGE_SIZE 512

extern const NTransgene::TProtoVersion ProtoVersion;

class TWorkerTarget;
class TWorkerVariables;

template <>
inline TString ToString<NProto::EMessageType>(const NProto::EMessageType& t) {
    switch (t) {
    case NProto::MT_UNKNOWN:                    return "unknown";
    case NProto::MT_ERROR:                      return "error";
    case NProto::MT_PING:                       return "ping";
    case NProto::MT_TEST:                       return "test";
    case NProto::MT_WORKERHELLO:                return "worker_hello";
    case NProto::MT_AUTHREPLY:                  return "auth_reply";
    case NProto::MT_AUTHSUCCESS:                return "auth_success";
    case NProto::MT_NEWCONFIG:                  return "new_config";
    case NProto::MT_FULLSTATUS:                 return "full_status";
    case NProto::MT_SINGLESTATUS:               return "single_status";
    case NProto::MT_THINSTATUS:                 return "thin_status";
    case NProto::MT_FULLCROSSNODE:              return "full_crossnode";
    case NProto::MT_SINGLECROSSNODE:            return "single_crossnode";
    case NProto::MT_COMMAND:                    return "command";
    case NProto::MT_POKE:                       return "poke";
    case NProto::MT_VARIABLES:                  return "variables";
    case NProto::MT_DISKSPACE:                  return "diskspace";
    case NProto::MT_MULTIPOKE:                  return "multipoke";
    case NProto::MT_COMMAND2:                   return "command2";
    case NProto::MT_NUMMESSAGES:                return "num_messages";
    default: ythrow yexception() << "bad message type";
    }
}

inline TString MessageTypeToString(int type) {
    return ToString<NProto::EMessageType>(static_cast<NProto::EMessageType>(type));
}

template <typename TMessage>
TString DebugStringForLog(const TMessage& message) {
    TString string = message.ShortDebugString();
    if (string.length() > 80) {
        string = string.substr(0, 77) + "...";
    }
    return MessageTypeToString(TMessage::Type) + " " + string;
}

struct TCommandMessage: NTransgene::TMessage<NProto::TCommandMessage, NProto::MT_COMMAND> {
    explicit TCommandMessage(const NTransgene::TPackedMessage& message)
        : TMessageBase(message)
    {
    }

    TCommandMessage(const TString& target, int task, ui32 flags, const TTaskState& state) {
        SetTarget(target);
        SetTask(task);
        SetFlags(flags);
        SetState(state.GetId());//FIXME!
    }

    //FIXME!
    TTaskState GetState() const {
        return TTaskState::Make(NProto::TCommandMessage::GetState(), NProto::TCommandMessage::GetStateValue());
    }

    static TString FormatText(const TString& target, int task, ui32 flags, const TTaskState& state);

    TString FormatText() const {
        return FormatText(GetTarget(), GetTask(), GetFlags(), GetState());
    }
};

struct TCommandMessage2: NTransgene::TMessage<NProto::TCommandMessage2, NProto::MT_COMMAND2> {
    explicit TCommandMessage2(const NTransgene::TPackedMessage& message)
        : TMessageBase(message)
    {
    }

    TCommandMessage2(const TString& target, ui32 flags) {
        SetTarget(target);
        SetFlags(flags);
        SetUnused(NProto::TS_UNDEFINED); //XXX: for backward
    }

    typedef google::protobuf::RepeatedField<int> TRepeatedTasks;

    static TString FormatText(const TString& target, const TRepeatedTasks& tasks, ui32 flags);

    TString FormatText() const {
        return FormatText(GetTarget(), GetTask(), GetFlags());
    }
};

template <>
inline TString ToString<TCommandMessage::ECommandFlags>(const TCommandMessage::ECommandFlags& s) {
    switch (s) {
    case TCommandMessage::CF_RUN:               return "run";
    case TCommandMessage::CF_INVALIDATE:        return "invalidate";
    case TCommandMessage::CF_MARK_SUCCESS:      return "mark_success";
    case TCommandMessage::CF_RESET_STAT:        return "reset_stat";
    case TCommandMessage::CF_FORCE_RUN:         return "force_run";
    case TCommandMessage::CF_FORCE_READY:       return "force_ready";
    case TCommandMessage::CF_RETRY:             return "retry";
    case TCommandMessage::CF_MARK_SKIPPED:      return "mark_skipped";
    case TCommandMessage::CF_CANCEL:            return "cancel";
    case TCommandMessage::CF_KILL:              return "kill";
    case TCommandMessage::CF_REMAIN_SUCCESS:    return "remain_success";
    case TCommandMessage::CF_RECURSIVE_UP:      return "recursive_up";
    case TCommandMessage::CF_RECURSIVE_DOWN:    return "recursive_down";
    case TCommandMessage::CF_RECURSIVE_ONLY:    return "recursive_only";
    default: ythrow yexception() << "bad command flag";
    }
}

template <>
inline TCommandMessage::ECommandFlags FromString<TCommandMessage::ECommandFlags>(const TString& s) {
    if (s == "run")             return TCommandMessage::CF_RUN;
    if (s == "invalidate")      return TCommandMessage::CF_INVALIDATE;
    if (s == "mark_success")    return TCommandMessage::CF_MARK_SUCCESS;
    if (s == "reset_stat")      return TCommandMessage::CF_RESET_STAT;
    if (s == "force_run")       return TCommandMessage::CF_FORCE_RUN;
    if (s == "force_ready")     return TCommandMessage::CF_FORCE_READY;
    if (s == "retry")           return TCommandMessage::CF_RETRY;
    if (s == "mark_skipped")    return TCommandMessage::CF_MARK_SKIPPED;
    if (s == "cancel")          return TCommandMessage::CF_CANCEL;
    if (s == "kill")            return TCommandMessage::CF_KILL;
    if (s == "remain_success")  return TCommandMessage::CF_REMAIN_SUCCESS;
    if (s == "recursive_up")    return TCommandMessage::CF_RECURSIVE_UP;
    if (s == "recursive_down")  return TCommandMessage::CF_RECURSIVE_DOWN;
    if (s == "recursive_only")  return TCommandMessage::CF_RECURSIVE_ONLY;
    ythrow yexception() << "bad command flag";
}

template <typename XY, typename XX>
void propagateStatisticsPart (XY A, XX B)
{
    A->SetLastStarted(B->GetLastStarted());
    A->SetLastFinished(B->GetLastFinished());
    A->SetLastSuccess(B->GetLastSuccess());
    A->SetLastFailure(B->GetLastFailure());
    A->SetLastDuration(B->GetLastDuration());
    A->SetLastChanged(B->GetLastChanged());
    A->SetStartedCounter(B->GetStartedCounter());
    A->SetToRepeat(B->GetToRepeat());
    A->SetRepeatMax(B->GetRepeatMax());
    A->SetPid(B->GetPid());
    A->SetLastProcStarted(B->GetLastProcStarted());

    A->SetCpuMax(B->GetCpuMax());
    A->SetCpuMaxDelta(B->GetCpuMaxDelta());
    A->SetCpuAvg(B->GetCpuAvg());
    A->SetCpuAvgDelta(B->GetCpuAvgDelta());
    A->SetMemMax(B->GetMemMax());
    A->SetMemMaxDelta(B->GetMemMaxDelta());
    A->SetMemAvg(B->GetMemAvg());
    A->SetMemAvgDelta(B->GetMemAvgDelta());
    A->SetIOMax(B->GetIOMax());
    A->SetIOMaxDelta(B->GetIOMaxDelta());
    A->SetIOAvg(B->GetIOAvg());
    A->SetIOAvgDelta(B->GetIOAvgDelta());
    A->SetUpdateCounter(B->GetUpdateCounter());
}

template<typename XX>
void propagateStatistics(NProto::TTaskStatus* A, const XX B)
{
    A->SetState(B->GetState().GetId());
    A->SetStateValue(B->GetState().GetValue());
    A->SetStateAfterCancel(B->GetStateAfterCancel().GetId());
    propagateStatisticsPart(A, B);
}

template<typename XX>
void propagateStatistics(XX A, const NProto::TTaskStatus* B)
{
    A->SetState(TTaskState::Make(B->GetState(), B->GetStateValue()));
    A->SetStateAfterCancel(TTaskState::Make(B->GetStateAfterCancel()));
    propagateStatisticsPart(A, B);
}

struct TFullStatusMessage: NTransgene::TMessage<NProto::TFullStatusMessage, NProto::MT_FULLSTATUS> {
    explicit TFullStatusMessage(const NTransgene::TPackedMessage& message)
        : TMessageBase(message)
    {
    }

    TFullStatusMessage() noexcept {
    }

    typedef google::protobuf::RepeatedPtrField<NProto::TTaskStatuses> TRepeatedTarget;
    typedef NProto::TTaskStatuses TTaskStatuses;
    typedef NProto::TTaskStatus TTaskStatus;
};

struct TSingleStatusMessage: NTransgene::TMessage<NProto::TSingleStatusMessage, NProto::MT_SINGLESTATUS> {
    explicit TSingleStatusMessage(const NTransgene::TPackedMessage& message)
        : TMessageBase(message)
    {
    }

    TSingleStatusMessage() noexcept {
    }
};

struct TThinStatusMessage: NTransgene::TMessage<NProto::TThinStatusMessage, NProto::MT_THINSTATUS> {
    explicit TThinStatusMessage(const NTransgene::TPackedMessage& message)
        : TMessageBase(message)
    {
    }

    TThinStatusMessage() noexcept {
    }
};

struct TAuthReplyMessage: NTransgene::TMessage<NProto::TAuthReplyMessage, NProto::MT_AUTHREPLY> {
    explicit TAuthReplyMessage(const NTransgene::TPackedMessage& message)
        : TMessageBase(message)
    {
    }

    TAuthReplyMessage(bool masterIsSecondary = false) {
        if (masterIsSecondary)
            SetMasterIsSecondary(true);
    }
};

struct TAuthSuccessMessage: NTransgene::TMessage<NProto::TAuthSuccessMessage, NProto::MT_AUTHSUCCESS> {
    explicit TAuthSuccessMessage(const NTransgene::TPackedMessage& message)
        : TMessageBase(message)
    {
    }

    TAuthSuccessMessage() noexcept {
    }
};

struct TWorkerHelloMessage: NTransgene::TMessage<NProto::TWorkerHelloMessage, NProto::MT_WORKERHELLO> {
    explicit TWorkerHelloMessage(const NTransgene::TPackedMessage& message)
        : TMessageBase(message)
    {
    }

    TWorkerHelloMessage(ui32 httpPort);

    void GenerateResponse(TString& output, const TString& keyPath) const;
};

struct TConfigMessage: NTransgene::TMessage<NProto::TConfigMessage, NProto::MT_NEWCONFIG> {
    explicit TConfigMessage(const NTransgene::TPackedMessage& message)
        : TMessageBase(message)
    {
    }

    TConfigMessage() noexcept {
    }

    typedef google::protobuf::RepeatedPtrField<NProto::TConfigMessage::TListType> TRepeatedLists;
    typedef google::protobuf::RepeatedPtrField<TString> TRepeatedItems;

};

struct TPokeMessage: NTransgene::TMessage<NProto::TPokeMessage, NProto::MT_POKE> {
    explicit TPokeMessage(const NTransgene::TPackedMessage& message)
        : TMessageBase(message)
    {
    }

    TPokeMessage(const TString& target, ui32 flags) {
        SetTarget(target);
        SetUnused(-1);
        SetFlags(flags);
    }

    static TString FormatText(const TString& target, ui32 flags);

    TString FormatText() const {
        return FormatText(GetTarget(), GetFlags());
    }
};

struct TMultiPokeMessage: NTransgene::TMessage<NProto::TMultiPokeMessage, NProto::MT_MULTIPOKE> {
    explicit TMultiPokeMessage(const NTransgene::TPackedMessage& message)
        : TMessageBase(message)
    {
    }

    TMultiPokeMessage(const TString& target, ui32 flags) {
        SetTarget(target);
        SetFlags(flags);
    }

    typedef google::protobuf::RepeatedField<int> TRepeatedTasks;

    static TString FormatText(const TString& target, const TRepeatedTasks& tasks, ui32 flags);

    TString FormatText() const {
        return FormatText(GetTarget(), GetTasks(), GetFlags());
    }
};

struct TErrorMessage: NTransgene::TMessage<NProto::TErrorMessage, NProto::MT_ERROR> {
    explicit TErrorMessage(const NTransgene::TPackedMessage& message)
        : TMessageBase(message)
    {
    }

    TErrorMessage(bool fatal, const TString& error) {
        SetFatal(fatal);
        SetError(error);
    }
};

struct TVariablesMessage: NTransgene::TMessage<NProto::TVariablesMessage, NProto::MT_VARIABLES> {
    explicit TVariablesMessage(const NTransgene::TPackedMessage& message)
        : TMessageBase(message)
    {
    }

    enum EVariableAction {
        VA_SET,
        VA_UNSET
    };

    TVariablesMessage() noexcept {
    }

    TVariablesMessage(const TWorkerVariables& variables);

    TVariablesMessage(const TString& name, const TString& value, EVariableAction action) {
        AddVariable(name, value, action);
    }

    typedef google::protobuf::RepeatedPtrField<NProto::TVariablesMessage::TVariable> TRepeatedVariable;

    void AddVariable(const TString& name, const TString& value, EVariableAction action);

    TString FormatText() const;
};

struct TDiskspaceMessage: NTransgene::TMessage<NProto::TDiskspaceMessage, NProto::MT_DISKSPACE> {
    explicit TDiskspaceMessage(const NTransgene::TPackedMessage& message)
        : TMessageBase(message)
    {
    }

    TDiskspaceMessage(const char *mountpoint);
};
