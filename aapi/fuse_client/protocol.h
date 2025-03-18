#pragma once

#include <contrib/libs/grpc/include/grpc++/create_channel.h>

#include <aapi/lib/proto/vcs.grpc.pb.h>

#include <library/cpp/deprecated/atomic/atomic.h>

#include <util/datetime/base.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/memory/blob.h>
#include <util/string/hex.h>
#include <util/system/event.h>
#include <util/thread/pool.h>


namespace NAapi {

NVcs::Vcs::Stub* CreateNewStub(const TString& proxyAddr);

template <class TFs>
class TObjectsRpc {
public:
    TObjectsRpc(const TString& proxyAddr, TFs* fs, int id)
        : ProxyAddr(proxyAddr)
        , Fs(fs)
        , Id(id)
        , Writer(new TThreadPool(TThreadPool::TParams().SetBlocking(true).SetCatching(false)))
    {
        Writer->Start(1);
    }

    ~TObjectsRpc() {
        Writer->SafeAddFunc([this]() {
            if (AtomicGet(RpcOpened)) {
                CloseRpc(true);
            }
        });
        Writer->Stop();
    }

    template <class TIter>
    void Write(TIter begin, TIter end) {
        TVector<TStringBuf> hashes(begin, end);

        Writer->SafeAddFunc([this, hashes = std::move(hashes)]() {
            if (AtomicGet(Destructor)) {
                return;
            }

            if (!AtomicGet(RpcOpened)) {
                OpenRpc();
            }

            THashes request;

            for (auto& h: hashes) {
                *request.AddHashes() = h;
            }

            Y_ENSURE(Stream->Write(request));
        });
    }

    template <class TCollection>
    void Write(const TCollection& c) {
        Write(c.begin(), c.end());
    }

    void Disconnect() {
        Writer->SafeAddFunc([this]() {
            if (AtomicGet(RpcOpened)) {
                CloseRpc(false);
            }
        });
    }

private:
    void OpenRpc() {
        Cerr << "Opening ObjectsRpc[" << Id << "]" << Endl;

        Stub.Reset(CreateNewStub(ProxyAddr));
        Ctx.Reset(new grpc::ClientContext());
        Stream.Reset(Stub->Objects3(Ctx.Get()).release());
        Reader.Reset(new TThreadPool(TThreadPool::TParams().SetBlocking(true).SetCatching(false)));
        Reader->Start(1);
        AtomicSet(RpcOpened, true);

        Reader->SafeAddFunc([this]() {
            TObjects objects;

            while (Stream->Read(&objects)) {
                if (!AtomicGet(RpcOpened)) {
                    continue;
                }

                for (size_t i = 0; i < objects.DataSize(); i += 2) {
                    if (objects.GetData(i).empty()) {
                        ythrow yexception() << "hash: " << HexEncode(objects.GetData(i + 1)) << ", error: " << objects.GetData(i + 2);
                    }
                    Fs->SetBlobData(objects.GetData(i), TBlob::FromString(objects.GetData(i + 1)));
                }
            }

            auto status = Stream->Finish();
            Y_ENSURE(status.ok(), status.error_message());
        });
    }

    void CloseRpc(bool destructor = false) {
        Cerr << "Closing ObjectsRpc[" << Id << "]" << Endl;

        AtomicSet(RpcOpened, false);
        if (destructor) {
            AtomicSet(Destructor, true);
        }
        Y_ENSURE(Stream->WritesDone());
        Reader->Stop();

        Stream.Destroy();
        Ctx.Destroy();
        Stub.Destroy();  // This really closes tcp connection with grpc-server
    }

private:
    TString ProxyAddr;
    TFs* Fs;
    int Id;

    THolder<IThreadPool> Writer;
    THolder<IThreadPool> Reader;

    TAtomic RpcOpened = false;
    TAtomic Destructor = false;

    THolder<NVcs::Vcs::Stub> Stub;
    THolder<grpc::ClientContext> Ctx;
    THolder<grpc::ClientReaderWriter<THashes, TObjects> > Stream;
};

template <class TFs, size_t N>
class TObjectsRpcs: IThreadFactory::IThreadAble {
public:
    TObjectsRpcs(const TString& proxyAddr, TFs* fs, ui64 keepaliveSeconds)
        : KeepaliveSeconds(keepaliveSeconds)
    {
        ui64 now = Now().Seconds();
        for (size_t i = 0; i < N; ++i) {
            Rpcs[i] = MakeHolder<TObjectsRpc<TFs>>(proxyAddr, fs, i);
            AtomicSet(Times[i], now);
        }
        Disconnector = SystemThreadFactory()->Run(this);
    }

    ~TObjectsRpcs() {
        Stopped.Signal();
        Disconnector->Join();
        for (size_t i = 0; i < N; ++i) {
            Rpcs[i].Destroy();
        }
    }

    template <class TIter>
    void Write(size_t i, TIter begin, TIter end) {
        AtomicSet(Times[i], static_cast<ui64>(Now().Seconds()));
        Rpcs[i]->Write(begin, end);
    }

    template <class TCollection>
    void Write(size_t i, const TCollection& c) {
        AtomicSet(Times[i], static_cast<ui64>(Now().Seconds()));
        Rpcs[i]->Write(c);
    }

private:
    void DoExecute() override {

        while (true) {
            if (Stopped.WaitT(TDuration::Seconds(KeepaliveSeconds))) {
                return;
            }

            ui64 now = Now().Seconds();

            for (size_t i = 0; i < N; ++i) {
                if (AtomicGet(Times[i]) + KeepaliveSeconds < now) {
                    Rpcs[i]->Disconnect();
                }
            }
        }
    }

private:
    ui64 KeepaliveSeconds;
    TAtomic Times[N];

    TAutoEvent Stopped;

    THolder<TObjectsRpc<TFs> > Rpcs[N];
    THolder<IThreadFactory::IThread> Disconnector;
};

}
