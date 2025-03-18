#include "graph_processor_base/graph_processor_base.h"

#include <library/cpp/tf/graph_processor_base-2.4/graph_def_multiproto/graph_def_multiproto.h>
#include <tensorflow/core/util/memmapped_file_system.h>
#include <util/generic/singleton.h>
#include <util/string/split.h>
#include <util/system/cpu_id.h>
#include <util/system/env.h>
#include <util/system/mutex.h>
#include <regex>


#ifdef GOOGLE_CUDA
#include <cuda_runtime.h>
#endif

#ifdef TENSORFLOW_WITH_XLA
#include <contrib/libs/tf-2.4/tensorflow/compiler/jit/flags.h>
#endif


using namespace NNeuralNet;


i32 NNeuralNet::GetCudaCompatibleNumDevices() {
    int numDevices = 0;
#ifdef GOOGLE_CUDA
    cudaError_t errorCode = cudaGetDeviceCount(&numDevices);
    Y_ENSURE(errorCode == cudaSuccess, "CUDA error: " << cudaGetErrorString(errorCode));
#endif
    return numDevices;
}

void NNeuralNet::CheckIsAvailibleCudaDeviceIdx(i32 cudaDeviceIdx) {
    i32 numCudaDevices = GetCudaCompatibleNumDevices();
    Y_ENSURE(numCudaDevices > 0, "There's no cuda compatible devices. Maybe TENSORFLOW_WITH_CUDA compilation flag was not set.");
    Y_ENSURE(cudaDeviceIdx >= 0, "Wrong cudaDeviceIdx requested");
    Y_ENSURE(cudaDeviceIdx < numCudaDevices, "Wrong cudaDeviceIdx requested");
}

////////////////////////////////////////////////////////////////////////////////
// Helpers
////////////////////////////////////////////////////////////////////////////////

static inline IOutputStream& operator<<(IOutputStream& o, const tf::Status& s) {
    return o << s.ToString();
}

////////////////////////////////////////////////////////////////////////////////
// TTfWorker
////////////////////////////////////////////////////////////////////////////////

TTfWorker::TTfWorker(TTfSession* session, const TStringList& inputNames, const TStringList& fetchNames, const TStringList& targetNames)
    : ParentSessionPtr_(session)
    , InputNames_(inputNames)
    , FetchNames_(fetchNames)
    , TargetNames_(targetNames)
{ }

const TTensorList& TTfWorker::Process(const TTensorList& inputTensors) {
    Y_ENSURE(InputNames_.size() == inputTensors.size(), "Invalid input: want " << InputNames_.size() << " items, got " << inputTensors.size() << " items");
    TVector<std::pair<TString, tf::Tensor>> inputs;
    for (size_t idx = 0; idx < InputNames_.size(); ++idx) {
        inputs.emplace_back(InputNames_[idx], inputTensors[idx]);
    }
    LastResults_.clear();
    tf::Status status = ParentSessionPtr_->Run(inputs, FetchNames_, TargetNames_, &LastResults_);
    Y_ENSURE(status.ok(), "Session::Run failed: " << status);
    return LastResults_;
}

const TStringList& TTfWorker::GetFetchNames() const {
    return FetchNames_;
}

const TTensorList& TTfWorker::GetLastResults() const {
    return LastResults_;
}

////////////////////////////////////////////////////////////////////////////////
// TTfSession
////////////////////////////////////////////////////////////////////////////////

TTfSession::TTfSession(TTfGraphProcessorBase* graph, const tf::SessionOptions& options)
    : ParentGraphProcessorBasePtr_(graph)
    , Session_(nullptr)
    , SessionOptions_(options)
{
    tf::Session* unsafeSessionPtr;
    tf::Status status = tf::NewSession(SessionOptions_, &unsafeSessionPtr);
    Y_ENSURE(status.ok(), "Cannot create session: " << status);

    Session_.Reset(unsafeSessionPtr);
}

TTfWorkerPtr TTfSession::MakeWorker(const TStringList& inputNames, const TStringList& fetchNames, const TStringList& targetNames) {
    //TODO: verify(inputNames are in graph)
    //TODO: verify(fetchNames are in graph)

    TTfWorkerPtr res(new TTfWorker(this, inputNames, fetchNames, targetNames));
    return res;
}

////////////////////////////////////////////////////////////////////////////////
// TTfGraphProcessorBase
////////////////////////////////////////////////////////////////////////////////

TTfGraphProcessorBase::TTfGraphProcessorBase(const TString& filename) {
    GraphDef_.Reset(new tf::GraphDef());
    tf::Status status;
    TString protoName;
    if (filename.EndsWith(".mmap")) {
        tf::MemmappedEnv* env;
        Env_.Reset(env = new tf::MemmappedEnv(tf::Env::Default()));
        status = env->InitializeFromFile(filename);
        Y_ENSURE(status.ok(), "Cannot read mmap " << filename << ": " << status);
        protoName = tf::MemmappedFileSystem::kMemmappedPackageDefaultGraphDef;
    } else {
        Env_.Reset(new tf::EnvWrapper(tf::Env::Default()));
        protoName = filename;
    }

    status = tf::ReadBinaryProto(Env_.Get(), protoName, GraphDef_.Get());
    Y_ENSURE(status.ok(), "Cannot read graph " << protoName << ": " << status);
}

TTfGraphProcessorBase::TTfGraphProcessorBase(IInputStream& stream) {
    Env_.Reset(new tf::EnvWrapper(tf::Env::Default()));
    GraphDef_.Reset(new tf::GraphDef());
    *GraphDef_ = NFTMoon::TGraphDefMultiprotoAdapter(stream).GetProto();
}

TTfGraphProcessorBasePtr TTfGraphProcessorBase::New(IInputStream& stream) {
    return new TTfGraphProcessorBase(stream);
}

TTfGraphProcessorBasePtr TTfGraphProcessorBase::New(const TString& filename) {
    return new TTfGraphProcessorBase(filename);
}

TTfGraphProcessorBase::~TTfGraphProcessorBase() noexcept {
    GraphDef_.Reset(nullptr);
    Env_.Reset(nullptr);
}

bool TTfGraphProcessorBase::HasOpMatching(const TString& regex) const {
    std::regex wanted(regex.c_str());
    for (int nodeIdx = 0; nodeIdx < GraphDef_->node_size(); ++nodeIdx) {
        if (std::regex_match(GraphDef_->node(nodeIdx).name().c_str(), wanted))
            return true;
    }
    return false;
}

tf::GraphDef& TTfGraphProcessorBase::GetGraphDef() {
    return *GraphDef_;
}

void TTfGraphProcessorBase::BindToDevice(i32 cudaDeviceIdx) {
    if (cudaDeviceIdx == TTfSession::DO_NOT_BIND_TO_DEVICE) {
        return;
    }

    TString deviceName;
    if (cudaDeviceIdx == TTfSession::CUDA_NO_DEVICE) {
        deviceName = TString("/cpu:0");
    } else {
        NNeuralNet::CheckIsAvailibleCudaDeviceIdx(cudaDeviceIdx);
        deviceName = TString("/gpu:") + ToString(cudaDeviceIdx);
    }

    if (!DeviceName.empty()) {
        Y_ENSURE(DeviceName == deviceName, "Trying to bind graph to device " << deviceName << " initially binded to " << DeviceName);
    }

    DeviceName = deviceName;

    auto& graphDef = GetGraphDef();
    for (int i = 0; i < graphDef.node_size(); ++i) {
        if (graphDef.mutable_node(i)->name() == DOSSIER_NODE_NAME) {
            continue;
        }
        graphDef.mutable_node(i)->set_device(DeviceName);
    }

}

tf::SessionOptions TTfGraphProcessorBase::BuildCommonSessionOptions(
        size_t numInterOpThreads,
        size_t numIntraOpThreads,
        i32 cudaDeviceIdx,
        bool allowSoftPlacement,
        bool allowGrowth,
        bool useXLA
) {
    tf::SessionOptions options;
    tf::ConfigProto& config = options.config;

    options.env = Env_.Get();
    config.set_inter_op_parallelism_threads(static_cast<int>(numInterOpThreads));
    config.set_intra_op_parallelism_threads(static_cast<int>(numIntraOpThreads));
    config.set_allow_soft_placement(allowSoftPlacement);
    config.mutable_gpu_options()->set_allow_growth(allowGrowth);

#ifdef TENSORFLOW_WITH_XLA
    if (useXLA) {
        auto* optimizer_options = config.mutable_graph_options()->mutable_optimizer_options();
        optimizer_options->set_global_jit_level(tensorflow::OptimizerOptions::ON_1);
        // These XLA flags are needed to trigger XLA properly from C (more generally
        // non-Python) clients. If this API is called again with `enable` set to
        // false, it is safe to keep these flag values as is.
        tf::MarkForCompilationPassFlags* flags = tf::GetMarkForCompilationPassFlags();
        flags->tf_xla_cpu_global_jit = true;
        flags->tf_xla_min_cluster_size = 1;
    }
#else
    Y_ENSURE(!useXLA, "Please rebuild with -DTENSORFLOW_WITH_XLA=1 to use xla");
#endif

    BindToDevice(cudaDeviceIdx);

    return options;
}

TTfSessionPtr TTfGraphProcessorBase::MakeSession(
        size_t numInterOpThreads,
        size_t numIntraOpThreads,
        i32 cudaDeviceIdx,
        bool allowSoftPlacement,
        bool allowGrowth,
        bool useXLA
) {
    const tf::SessionOptions& options = BuildCommonSessionOptions(numInterOpThreads, numIntraOpThreads, cudaDeviceIdx, allowSoftPlacement, allowGrowth, useXLA);
    return MakeSession(options);
}

TTfSessionPtr TTfGraphProcessorBase::MakeSession(const tf::SessionOptions& options) {
    TTfSessionPtr session(new TTfSession(this, options));
    tf::Status status = session->Session_->Create(*GraphDef_);
    Y_ENSURE(status.ok(), "Cannot import graph: " << status);
    return session;
}

