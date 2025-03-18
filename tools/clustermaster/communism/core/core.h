#pragma once

#include <tools/clustermaster/communism/core/core.pb.h>

#include <library/cpp/deprecated/transgene/transgene.h>

#include <util/datetime/base.h>
#include <util/generic/intrlist.h>
#include <util/generic/noncopyable.h>
#include <util/generic/ptr.h>
#include <util/generic/yexception.h>
#include <util/network/sock.h>
#include <util/system/mutex.h>

#include <utility>

namespace NCommunism {
namespace NCore {

using NTransgene::TProtoVersion;

using NTransgene::TBadVersion;
using NTransgene::TDisconnection;

using NTransgene::TMessage;
using NTransgene::TPackedMessage;
using NTransgene::TPackedMessageList;

typedef ui32 TActionId;
typedef ui32 TKey;

const TProtoVersion ProtoVersion = 6;
const TIpPort DefaultPort = 3366;

#ifdef COMMUNISM_SOLVER
const TDuration HeartbeatTimeout = TDuration::Max();
#else // For temporary protocol compatibility purpose only
const TDuration HeartbeatTimeout = TDuration::Seconds(30);
#endif

const TDuration ReconnectTimeout = TDuration::Seconds(1);
const TDuration NetworkLatency = TDuration::Seconds(5);
const TDuration KeepaliveIdle = TDuration::Seconds(30); // idle time before keepalive probes are sent
const TDuration KeepaliveIntvl = TDuration::Seconds(2); // interval between keepalive probes
const int KeepaliveCnt = 8; // number of keepalive probes

template <class T>
class TTransceiver: public NTransgene::TTransceiver<TFakeMutex> {
private:
    typedef NTransgene::TTransceiver<TFakeMutex> TBase;

    T Processor;

    THolder<TStreamSocket> Socket;

public:
    TTransceiver(typename TTypeTraits<T>::TFuncParam processor, TAutoPtr<TStreamSocket> socket, size_t maxBytesPerRecvSend)
        : TBase(ProtoVersion)
        , Processor(processor)
        , Socket(socket)
    {
        SetMaxBytesPerRecvSend(maxBytesPerRecvSend);
    }

    const TStreamSocket* GetSocket() const noexcept {
        return Socket.Get();
    }

    void ResetSocket(TStreamSocket* socket) noexcept {
        Socket.Reset(socket);
        ResetState();
    }

    size_t Recv() {
        return TBase::Recv(Socket.Get(), Processor);
    }

    size_t Send() {
        return TBase::Send(Socket.Get());
    }
};

struct THeartbeatMessage: TMessage<NProto::THeartbeatMessage, NProto::MT_HEARTBEAT> {
    explicit THeartbeatMessage(const TPackedMessage& message)
        : TMessageBase(message)
    {
    }

    THeartbeatMessage() {
    }
};

struct TDefineMessage: TMessage<NProto::TDefineMessage, NProto::MT_DEFINE> {
    explicit TDefineMessage(const TPackedMessage& message)
        : TMessageBase(message)
    {
    }

    explicit TDefineMessage(TActionId actionId, TKey key, const NProto::TDefinition& definition) {
        SetActionId(actionId);
        SetKey(key);
        MutableDefinition()->CopyFrom(definition);
    }

    explicit TDefineMessage(TActionId actionId, TKey key, TKey origin) {
        SetActionId(actionId);
        SetKey(key);
        SetOrigin(origin);
    }
};

struct TUndefMessage: TMessage<NProto::TUndefMessage, NProto::MT_UNDEF> {
    explicit TUndefMessage(const TPackedMessage& message)
        : TMessageBase(message)
    {
    }

    explicit TUndefMessage(TKey key) {
        SetKey(key);
    }
};

struct TRequestMessage: TMessage<NProto::TRequestMessage, NProto::MT_REQUEST> {
    explicit TRequestMessage(const TPackedMessage& message)
        : TMessageBase(message)
    {
    }

    explicit TRequestMessage(TActionId actionId, TKey key) {
        SetActionId(actionId);
        SetKey(key);
    }
};

struct TClaimMessage: TMessage<NProto::TClaimMessage, NProto::MT_CLAIM> {
    explicit TClaimMessage(const TPackedMessage& message)
        : TMessageBase(message)
    {
    }

    explicit TClaimMessage(TActionId actionId, TKey key) {
        SetActionId(actionId);
        SetKey(key);
    }
};

struct TDisclaimMessage: TMessage<NProto::TDisclaimMessage, NProto::MT_DISCLAIM> {
    explicit TDisclaimMessage(const TPackedMessage& message)
        : TMessageBase(message)
    {
    }

    explicit TDisclaimMessage(TKey key) {
        SetKey(key);
    }
};

struct TDetailsMessage: TMessage<NProto::TDetailsMessage, NProto::MT_DETAILS> {
    explicit TDetailsMessage(const TPackedMessage& message)
        : TMessageBase(message)
    {
    }

    TDetailsMessage(TKey key, const NProto::TDetails& details) {
        SetKey(key);
        MutableDetails()->CopyFrom(details);
    }
};

struct TGrantMessage: TMessage<NProto::TGrantMessage, NProto::MT_GRANT> {
    explicit TGrantMessage(const TPackedMessage& message)
        : TMessageBase(message)
    {
    }

    explicit TGrantMessage(TActionId actionId, TKey key) {
        SetActionId(actionId);
        SetKey(key);
    }
};

struct TRejectMessage: TMessage<NProto::TRejectMessage, NProto::MT_REJECT> {
    explicit TRejectMessage(const TPackedMessage& message)
        : TMessageBase(message)
    {
    }

    explicit TRejectMessage(TActionId actionId, TKey key) {
        SetActionId(actionId);
        SetKey(key);
    }

    explicit TRejectMessage(TActionId actionId, TKey key, const TString& message) {
        SetActionId(actionId);
        SetKey(key);
        SetMessage(message);
    }
};

struct TGroupMessage: TMessage<NProto::TGroupMessage, NProto::MT_GROUP> {
    explicit TGroupMessage(const TPackedMessage& message)
        : TMessageBase(message)
    {
    }

    explicit TGroupMessage(size_t count) {
        SetCount(count);
        Y_VERIFY(static_cast<size_t>(GetCount()) == count, "integer overflow");
    }
};

}
}
