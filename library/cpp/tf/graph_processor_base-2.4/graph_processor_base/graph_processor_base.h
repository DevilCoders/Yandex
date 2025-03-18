#pragma once

#include <tensorflow/core/public/session.h>
#include <tensorflow/core/util/tensor_format.h>
#include <util/generic/ptr.h>
#include <util/generic/vector.h>

namespace tf = tensorflow;

namespace NNeuralNet {

using TStringList = TVector<TString>;
using TTensorList = TVector<tf::Tensor>;
template <typename T>
using TSmartPtr = TIntrusivePtr<T>;
class TTfWorker;
using TTfWorkerPtr = TSmartPtr<TTfWorker>;
class TTfSession;
using TTfSessionPtr = TSmartPtr<TTfSession>;
class TTfGraphProcessorBase;
using TTfGraphProcessorBasePtr = TSmartPtr<TTfGraphProcessorBase>;


i32 GetCudaCompatibleNumDevices();
void CheckIsAvailibleCudaDeviceIdx(i32 cudaDeviceIdx);

const char* const DOSSIER_NODE_NAME = "Dossier";

class TTfWorker: public TAtomicRefCount<TTfWorker> {
    friend class TTfSession; // TTfWorker factory
    TTfWorker( TTfSession* session,
        const TStringList& inputNames,
        const TStringList& fetchNames,
        const TStringList& targetNames
    );

public:
    virtual ~TTfWorker() noexcept = default;

    // Generic
    const TTensorList& Process(const TTensorList& inputs);
    const TStringList& GetFetchNames() const;
    const TTensorList& GetLastResults() const;

private:
    TTfSessionPtr ParentSessionPtr_;
    TStringList InputNames_;
    TStringList FetchNames_;
    TStringList TargetNames_;
    TTensorList LastResults_;
};

class TTfSession: public TAtomicRefCount<TTfSession> {
    friend class TTfGraphProcessorBase; // TTfSession factory
    TTfSession(TTfGraphProcessorBase* graph, const tf::SessionOptions& options);

public:
    static constexpr i32 CUDA_NO_DEVICE = -1;
    static constexpr i32 DO_NOT_BIND_TO_DEVICE = -2;

public:
    virtual ~TTfSession() noexcept = default;

    template <typename... TArgs>
    tf::Status Run(TArgs&&... args) {
        return Session_->Run(std::forward<TArgs>(args)...);
    }

    TTfWorkerPtr MakeWorker(
        const TStringList& inputNames,
        const TStringList& fetchNames,
        const TStringList& targetNames = {}
    );

private:
    TTfGraphProcessorBasePtr ParentGraphProcessorBasePtr_;
    TAtomicSharedPtr<tf::Session> Session_;
    tf::SessionOptions SessionOptions_;
};

class TTfGraphProcessorBase: public TAtomicRefCount<TTfGraphProcessorBase> {
protected:
    TTfGraphProcessorBase(const TString& filename);
    TTfGraphProcessorBase(IInputStream& input);

public:
    static TTfGraphProcessorBasePtr New(const TString& filename);
    static TTfGraphProcessorBasePtr New(IInputStream& input);
    virtual ~TTfGraphProcessorBase() noexcept;

    bool HasOpMatching(const TString& regex) const;

    tf::GraphDef& GetGraphDef();

    virtual tf::SessionOptions BuildCommonSessionOptions(
        size_t numInterOpThreads,
        size_t numIntraOpThreads,
        i32 cudaDeviceIdx,
        bool allowSoftPlacement = false,
        bool allowGrowth = false,
        bool useXLA = false
    );
    TTfSessionPtr MakeSession(
        size_t numInterOpThreads,
        size_t numIntraOpThreads,
        i32 cudaDeviceIdx = TTfSession::CUDA_NO_DEVICE,
        bool allowSoftPlacement = false,
        bool allowGrowth = false,
        bool useXLA = false
    );

    TTfSessionPtr MakeSession(const tf::SessionOptions& options);
    
private:
    void BindToDevice(i32 cudaDeviceIdx);

private:
    TString DeviceName;

protected:
    THolder<tf::GraphDef> GraphDef_;
    THolder<tf::Env> Env_;
};

} // namespace NNeuralNet
