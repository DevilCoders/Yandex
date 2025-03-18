#pragma once

#include <util/generic/intrlist.h>
#include <util/generic/noncopyable.h>
#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/generic/yexception.h>
#include <util/network/sock.h>
#include <util/system/error.h>

#include <contrib/libs/protobuf/src/google/protobuf/message_lite.h>

namespace NTransgene {
    using TMessageType = ui8;
    using TMessageSize = ui32;

    using TProtoGeneration = ui8;
    using TProtoVersion = ui32;

    struct TBadVersion : yexception {};

    struct TDisconnection : yexception {
        size_t Recvd;
        size_t Sent;

        TDisconnection() noexcept
            : Recvd(0)
            , Sent(0)
        {
        }
    };

    namespace NPrivate {
        const TProtoGeneration ProtoGeneration = 4;

        inline size_t CheckRecv(ssize_t ret) {
            if (ret > 0) {
                return ret;
            } else if (ret == 0) {
                throw TDisconnection() << TStringBuf("connection closed");
            } else if (-ret == EAGAIN || -ret == EINTR) {
                return 0;
            } else {
                throw TDisconnection() << TStringBuf("recv: ") << LastSystemErrorText(-ret);
            }
        }

        inline size_t CheckSend(ssize_t ret) {
            if (ret >= 0) {
                return ret;
            } else if (-ret == EAGAIN || -ret == EINTR) {
                return 0;
            } else {
                throw TDisconnection() << TStringBuf("send: ") << LastSystemErrorText(-ret);
            }
        }

        template <class T1, class T2>
        inline void IncDec(T1& inc, T2& dec, size_t cnt) {
            inc += cnt;
            dec -= cnt;
        }

    }

    template <class T, TMessageType TYPE>
    class TMessage;

    class TPackedMessage: public TIntrusiveListItem<TPackedMessage> {
    private:
        template <class T, TMessageType TYPE>
        friend class TMessage;

        struct THeader {
        private:
            ui8 Bytes[sizeof(TMessageType) + sizeof(TMessageSize)];

        public:
            inline const ui8* Data() const noexcept {
                return reinterpret_cast<const ui8*>(&Bytes);
            }

            inline ui8* Data() noexcept {
                return reinterpret_cast<ui8*>(&Bytes);
            }

            inline size_t Size() const noexcept {
                return sizeof(Bytes);
            }

            inline unsigned GetMessageType() const noexcept {
                return InetToHost(*reinterpret_cast<const TMessageType*>(Data()));
            }

            inline void SetMessageType(TMessageType value) noexcept {
                *reinterpret_cast<TMessageType*>(Data()) = HostToInet(value);
            }

            inline size_t GetMessageSize() const noexcept {
                return InetToHost(*reinterpret_cast<const TMessageSize*>(Data() + sizeof(TMessageType)));
            }

            inline void SetMessageSize(TMessageSize value) noexcept {
                *reinterpret_cast<TMessageSize*>(Data() + sizeof(TMessageType)) = HostToInet(value);
            }
        };

        THeader Header;

        TAtomicSharedPtr<ui8, TDeleteArray> Data;

        size_t Recvd;
        size_t Sent;

        template <class T, TMessageType TYPE>
        TPackedMessage(const TMessage<T, TYPE>& message)
            : Recvd(0)
            , Sent(0)
        {
            if (static_cast<size_t>(message.ByteSize()) > static_cast<size_t>(Max<TMessageSize>())) {
                ythrow yexception() << TStringBuf("message is too big");
            }
            Header.SetMessageType(TMessage<T, TYPE>::Type);
            Header.SetMessageSize(message.GetCachedSize());
            if (Header.GetMessageSize()) {
                Data.Reset(new ui8[Header.GetMessageSize()]);
                message.SerializeWithCachedSizesToArray(Data.Get());
            }
            Recvd = Header.Size() + Header.GetMessageSize();
        }

    public:
        inline TPackedMessage() noexcept
            : Recvd(0)
            , Sent(0)
        {
        }

        inline bool FullyRecvd() const noexcept {
            return Recvd >= Header.Size() && Recvd == Header.Size() + Header.GetMessageSize();
        }

        inline bool FullySent() const noexcept {
            return Sent >= Header.Size() && Sent == Header.Size() + Header.GetMessageSize();
        }

        inline void ResetRecvd() noexcept {
            Recvd = 0;
            Data.Reset(nullptr);
        }

        inline void ResetSent() noexcept {
            Sent = 0;
        }

        inline unsigned GetType() const noexcept {
            Y_VERIFY(FullyRecvd(), "TPackedMessage is not fully recvd");
            return Header.GetMessageType();
        }

        inline size_t GetSize() const noexcept {
            Y_VERIFY(FullyRecvd(), "TPackedMessage is not fully recvd");
            return Header.GetMessageSize();
        }

        TAutoPtr<TPackedMessage> Clone() const {
            if (!FullyRecvd()) {
                ythrow yexception() << TStringBuf("TPackedMessage is not fully recvd");
            }

            TAutoPtr<TPackedMessage> message(new TPackedMessage);

            message->Recvd = Recvd;
            message->Header = Header;
            message->Data = Data;

            return message;
        }

        bool Recv(TStreamSocket* socket, size_t& bytesLeft) {
            if (FullyRecvd()) {
                return true;
            }

            if (!socket) {
                ythrow TSystemError(EINVAL) << TStringBuf("null socket pointer specified");
            }

            if (Recvd < Header.Size() && Data.Get()) {
                Data.Reset(nullptr);
            }

            if (bytesLeft && Recvd < Header.Size()) {
                NPrivate::IncDec(Recvd, bytesLeft, NPrivate::CheckRecv(socket->Recv(Header.Data() + Recvd, Min<size_t>(bytesLeft, Header.Size() - Recvd))));
            }

            if (Recvd == Header.Size() && Header.GetMessageSize() && !Data.Get()) {
                Data.Reset(new ui8[Header.GetMessageSize()]);
            }

            if (bytesLeft && Recvd >= Header.Size() && Header.GetMessageSize()) {
                NPrivate::IncDec(Recvd, bytesLeft, NPrivate::CheckRecv(socket->Recv(Data.Get() + (Recvd - Header.Size()), Min<size_t>(bytesLeft, Header.Size() + Header.GetMessageSize() - Recvd))));
            }

            return FullyRecvd();
        }

        bool Send(TStreamSocket* socket, size_t& bytesLeft) {
            if (FullySent()) {
                return true;
            }

            if (!socket) {
                ythrow TSystemError(EINVAL) << TStringBuf("null socket pointer specified");
            }

            if (bytesLeft && Sent < Header.Size()) {
                NPrivate::IncDec(Sent, bytesLeft, NPrivate::CheckSend(socket->Send(Header.Data() + Sent, Min<size_t>(bytesLeft, Min<size_t>(Recvd, Header.Size()) - Sent))));
            }

            if (bytesLeft && Sent >= Header.Size() && Header.GetMessageSize()) {
                NPrivate::IncDec(Sent, bytesLeft, NPrivate::CheckSend(socket->Send(Data.Get() + (Sent - Header.Size()), Min<size_t>(bytesLeft, Min<size_t>(Recvd, Header.Size() + Header.GetMessageSize()) - Sent))));
            }

            return FullySent();
        }

        inline bool Recv(TStreamSocket* socket) {
            size_t bytesLeft = Max<size_t>();
            return Recv(socket, bytesLeft);
        }

        inline bool Send(TStreamSocket* socket) {
            size_t bytesLeft = Max<size_t>();
            return Send(socket, bytesLeft);
        }
    };

    struct TPackedMessageList : TIntrusiveListWithAutoDelete<TPackedMessage, TDelete> {};

    template <class T, TMessageType TYPE>
    class TMessage: public T {
    public:
        static const unsigned Type = TYPE;

    protected:
        typedef TMessage TMessageBase;

        inline TMessage() = default;

        TMessage(const TPackedMessage& message) {
            if (!message.FullyRecvd()) {
                ythrow yexception() << TStringBuf("TPackedMessage is not fully recvd");
            }
            if (message.Header.GetMessageType() != Type) {
                ythrow yexception() << TStringBuf("bad message type: want ") << static_cast<const TMessageType>(Type) << TStringBuf(", got ") << message.Header.GetMessageType();
            }
            if (message.Header.GetMessageSize()) {
                Y_PROTOBUF_SUPPRESS_NODISCARD T::ParseFromArray(message.Data.Get(), message.Header.GetMessageSize());
            }
        }

        ~TMessage() override = default;

    public:
        inline TAutoPtr<TPackedMessage> Pack() const {
            return new TPackedMessage(*this);
        }
    };

    template <class TMUTEX>
    class TTransceiver: public TNonCopyable {
    protected:
        TMUTEX Mutex;

        struct THeader : TNonCopyable {
        private:
            ui8 Bytes[sizeof(TProtoGeneration) + sizeof(TProtoVersion)];

        public:
            inline const ui8* Data() const noexcept {
                return reinterpret_cast<const ui8*>(&Bytes);
            }

            inline ui8* Data() noexcept {
                return reinterpret_cast<ui8*>(&Bytes);
            }

            inline size_t Size() const noexcept {
                return sizeof(Bytes);
            }

            inline unsigned GetProtoGeneration() const noexcept {
                return InetToHost(*reinterpret_cast<const TProtoGeneration*>(Data()));
            }

            inline void SetProtoGeneration(TProtoGeneration value) noexcept {
                *reinterpret_cast<TProtoGeneration*>(Data()) = HostToInet(value);
            }

            inline unsigned GetProtoVersion() const noexcept {
                return InetToHost(*reinterpret_cast<const TProtoVersion*>(Data() + sizeof(TProtoGeneration)));
            }

            inline void SetProtoVersion(TProtoVersion value) noexcept {
                *reinterpret_cast<TProtoVersion*>(Data() + sizeof(TProtoGeneration)) = HostToInet(value);
            }
        };

        THeader ThisHeader;
        THeader PeerHeader;

        size_t MaxBytesPerRecvSend;

        TPackedMessageList Incoming;
        TPackedMessageList Outgoing;

        size_t GroupedIncoming;

        size_t HeaderRecvd;
        size_t HeaderSent;

        inline bool HeadersRecvdSent() const noexcept {
            return HeaderRecvd == PeerHeader.Size() && HeaderSent == ThisHeader.Size();
        }

        inline void CheckHeaders() const {
            if (HeadersRecvdSent() && (PeerHeader.GetProtoGeneration() != ThisHeader.GetProtoGeneration() || PeerHeader.GetProtoVersion() != ThisHeader.GetProtoVersion())) {
                throw TBadVersion() << PeerHeader.GetProtoGeneration() << '.' << PeerHeader.GetProtoVersion() << TStringBuf(", expected ") << ThisHeader.GetProtoGeneration() << '.' << ThisHeader.GetProtoVersion();
            }
        }

    public:
        inline TTransceiver(TProtoVersion protoVersion) noexcept
            : MaxBytesPerRecvSend(Max<size_t>())
            , GroupedIncoming(0)
            , HeaderRecvd(0)
            , HeaderSent(0)
        {
            ThisHeader.SetProtoGeneration(NPrivate::ProtoGeneration);
            ThisHeader.SetProtoVersion(protoVersion);
        }

        virtual ~TTransceiver() = default;

        inline void SetMaxBytesPerRecvSend(size_t bytes) noexcept {
            TGuard<TMUTEX> guard(Mutex);

            MaxBytesPerRecvSend = bytes;
        }

        void ResetState() noexcept {
            TGuard<TMUTEX> guard(Mutex);

            HeaderRecvd = 0;
            HeaderSent = 0;

            if (!Incoming.Empty()) {
                Incoming.RBegin()->ResetRecvd();
            }
            if (!Outgoing.Empty()) {
                Outgoing.Begin()->ResetSent();
            }
        }

        void ResetQueue() noexcept {
            TGuard<TMUTEX> guard(Mutex);

            Incoming.Clear();
            Outgoing.Clear();
        }

        void Enqueue(TAutoPtr<TPackedMessage> message) noexcept {
            Y_VERIFY(message->FullyRecvd(), "TPackedMessage is not fully recvd");
            message->ResetSent();

            TGuard<TMUTEX> guard(Mutex);
            Outgoing.PushBack(message.Release());
        }

        void Enqueue(TPackedMessageList& list) noexcept {
            TGuard<TMUTEX> guard(Mutex);

            while (!list.Empty()) {
                Enqueue(list.PopFront());
            }
        }

        inline bool Enqueued() const noexcept {
            TGuard<TMUTEX> guard(Mutex);

            return !Outgoing.Empty();
        }

        inline bool HasDataToSend() const noexcept {
            TGuard<TMUTEX> guard(Mutex);

            return HeaderSent < ThisHeader.Size() || (HeaderRecvd == PeerHeader.Size() && Enqueued());
        }

        inline void GroupIncoming(size_t count) noexcept {
            TGuard<TMUTEX> guard(Mutex);

            Y_VERIFY(GroupedIncoming + count >= GroupedIncoming, "size_t overflow");
            GroupedIncoming += count;
        }

        template <class TProcessor>
        size_t Recv(TStreamSocket* socket, TProcessor& processor) {
            TGuard<TMUTEX> guard1(Mutex);

            size_t bytesLeft = MaxBytesPerRecvSend;

            if (HeaderRecvd < PeerHeader.Size()) {
                NPrivate::IncDec(HeaderRecvd, bytesLeft, NPrivate::CheckRecv(socket->Recv(PeerHeader.Data() + HeaderRecvd, Min<size_t>(bytesLeft, PeerHeader.Size() - HeaderRecvd))));
                CheckHeaders();
            }

            if (!HeadersRecvdSent()) {
                return MaxBytesPerRecvSend - bytesLeft;
            }

            if (Incoming.Empty()) {
                Incoming.PushBack(new TPackedMessage);
            }

            try {
                if (Incoming.RBegin()->Recv(socket, bytesLeft))
                    do {
                        if (!GroupedIncoming || !--GroupedIncoming) {
                            while (!Incoming.Empty()) {
                                TAutoPtr<TPackedMessage> message(Incoming.PopFront());
                                TGuard<TMUTEX, TInverseLockOps<TCommonLockOps<TMUTEX>>> guard2(Mutex);
                                processor(message);
                            }
                        }
                        Incoming.PushBack(new TPackedMessage);
                    } while (bytesLeft && Incoming.RBegin()->Recv(socket, bytesLeft));
            } catch (TDisconnection& e) {
                e.Recvd = MaxBytesPerRecvSend - bytesLeft;
                throw;
            }

            return MaxBytesPerRecvSend - bytesLeft;
        }

        size_t Send(TStreamSocket* socket) {
            TGuard<TMUTEX> guard(Mutex);

            size_t bytesLeft = MaxBytesPerRecvSend;

            if (HeaderSent < ThisHeader.Size()) {
                NPrivate::IncDec(HeaderSent, bytesLeft, NPrivate::CheckSend(socket->Send(ThisHeader.Data() + HeaderSent, Min<size_t>(bytesLeft, ThisHeader.Size() - HeaderSent))));
                CheckHeaders();
            }

            if (!HeadersRecvdSent()) {
                return MaxBytesPerRecvSend - bytesLeft;
            }

            if (Outgoing.Empty()) {
                return MaxBytesPerRecvSend - bytesLeft;
            }

            try {
                if (Outgoing.Begin()->Send(socket, bytesLeft))
                    do {
                        delete Outgoing.PopFront();
                    } while (bytesLeft && !Outgoing.Empty() && Outgoing.Begin()->Send(socket, bytesLeft));
            } catch (TDisconnection& e) {
                e.Sent = MaxBytesPerRecvSend - bytesLeft;
                throw;
            }

            return MaxBytesPerRecvSend - bytesLeft;
        }
    };

}
