#include "tensorflow/core/common_runtime/executor.h"
#include "tensorflow/core/common_runtime/function.h"
#include "tensorflow/core/framework/memory_types.h"
#include "tensorflow/core/framework/node_def_builder.h"
#include "tensorflow/core/framework/op_kernel.h"
#include "tensorflow/core/lib/core/status.h"

#include <util/generic/vector.h>
#include <util/system/cpu_id.h>

// This code is a major copy-paste from function.cc

namespace {
    using namespace tensorflow;

    const string _KERNEL = "_kernel";

    class TPerisaOp: public OpKernel {
    public:
        TPerisaOp(FunctionLibraryRuntime::Handle handle, OpKernelConstruction* ctx)
            : OpKernel(ctx)
            , Handle(handle)
        {
        }

        ~TPerisaOp() override {
        }

        void Compute(OpKernelContext* ctx) override {
            FunctionLibraryRuntime* lib = ctx->function_library();
            OP_REQUIRES(ctx, lib != nullptr, errors::Internal("No flr"));
            FunctionLibraryRuntime::Options opts;
            opts.step_id = ctx->step_id();
            opts.rendezvous = ctx->rendezvous();
            opts.cancellation_manager = ctx->cancellation_manager();
            opts.step_container = ctx->step_container();
            opts.stats_collector = ctx->stats_collector();
            opts.runner = ctx->runner();
            TVector<Tensor> args;
            args.reserve(ctx->num_inputs());
            for (int i = 0; i < ctx->num_inputs(); ++i) {
                args.push_back(ctx->input(i));
            }
            TVector<Tensor>* rets = new TVector<Tensor>;
            lib->Run(opts, Handle, args, rets, [ctx, rets](const Status& status) {
                if (!status.ok()) {
                    ctx->SetStatus(status);
                } else {
                    const int ret_size = static_cast<int>(rets->size());
                    CHECK_EQ(ret_size, ctx->num_outputs());
                    for (int i = 0; i < ret_size; ++i) {
                        ctx->set_output(i, (*rets)[i]);
                    }
                }
                delete rets;
            });
        }

    private:
        FunctionLibraryRuntime::Handle Handle;
        TF_DISALLOW_COPY_AND_ASSIGN(TPerisaOp);
    };

    Status CreatePerisaOpKernel(FunctionLibraryRuntime*, const NodeDef&, std::unique_ptr<OpKernel>*);
    extern "C" void RegisterBiasOpAvx();
    extern "C" void RegisterCwiseOpMul1Avx();
    extern "C" void RegisterMatMulOpAvx();

    struct TPerisaRegistrator {
        TPerisaRegistrator() {
            RegisterDefaultCustomKernelCreator(CreatePerisaOpKernel);
            RegisterBiasOpAvx();
            RegisterCwiseOpMul1Avx();
            RegisterMatMulOpAvx();
        }
    } perisaRegistrator;

    const char* PERISA_OPS[] = {"BiasAdd", "MatMul", "Mul"};

    Status CreatePerisaOpKernel(FunctionLibraryRuntime* flr, const NodeDef& nodeDef, std::unique_ptr<OpKernel>* kernel) {
        bool available = false;
        for (auto* op : PERISA_OPS) {
            if (nodeDef.op() == op) {
                available = true;
                break;
            }
        }
        if (!available) {
            return errors::Unavailable("Perisa: no kernels for op ", nodeDef.op());
        }

        AttrSlice attrs(&nodeDef.attr());
        const AttrValue* attrKernel = attrs.Find(_KERNEL);
        if (attrKernel && attrKernel->s()) {
            return errors::InvalidArgument("Perisa: already labelled op ", nodeDef.op());
        }

        NodeDef nodeDefPerIsa = nodeDef;
        Status s = errors::Unavailable("Perisa: no kernels for op ", nodeDef.op());

        const FunctionLibraryDefinition* flrDef = flr->GetFunctionLibraryDefinition();
        if (flrDef->Find(nodeDef.op()) == nullptr) {
            if (NX86::CachedHaveAVX()) {
                (*nodeDefPerIsa.mutable_attr())[_KERNEL].set_s("avx");
                OpKernel* localKernel = nullptr;
                s = CreateNonCachedKernel(flr->device(), flr, nodeDefPerIsa, flr->graph_def_version(), &localKernel);
                kernel->reset(localKernel);
                if (s.ok()) {
                    VLOG(1) << "Perisa: install AVX kernel for " << nodeDef.name() << " = " << nodeDef.op();
                }
            }
            if (!s.ok()) {
                (*nodeDefPerIsa.mutable_attr())[_KERNEL].clear_s();
                OpKernel* localKernel = nullptr;
                s = CreateNonCachedKernel(flr->device(), flr, nodeDefPerIsa, flr->graph_def_version(), &localKernel);
                kernel->reset(localKernel);
                if (s.ok()) {
                    VLOG(1) << "Perisa: install ref kernel for " << nodeDef.name() << " = " << nodeDef.op();
                }
            }
            if (!s.ok()) {
                VLOG(1) << "Perisa: install no kernel for " << nodeDef.name() << " = " << nodeDef.op() << ", " << s.ToString();
            }
            return s;
        }

        FunctionLibraryRuntime::Handle handle;
        if (NX86::CachedHaveAVX()) {
            (*nodeDefPerIsa.mutable_attr())[_KERNEL].set_s("avx");
            s = flr->Instantiate(nodeDefPerIsa.op(), AttrSlice(&nodeDefPerIsa.attr()), &handle);
            if (s.ok()) {
                VLOG(1) << "Perisa: install AVX kernel for " << nodeDef.name() << " = " << nodeDef.op();
            }
        }
        if (!s.ok()) {
            (*nodeDefPerIsa.mutable_attr())[_KERNEL].clear_s();
            s = flr->Instantiate(nodeDefPerIsa.op(), AttrSlice(&nodeDefPerIsa.attr()), &handle);
            if (s.ok()) {
                VLOG(1) << "Perisa: install ref kernel for " << nodeDef.name() << " = " << nodeDef.op();
            }
        }
        if (!s.ok()) {
            VLOG(1) << "Perisa: install no kernel for " << nodeDef.name() << " = " << nodeDef.op() << ", " << s.ToString();
            return s;
        }

        const FunctionBody* fbody = flr->GetFunctionBody(handle);
        CHECK_NOTNULL(fbody);

        MemoryTypeVector input_memory_types;
        MemoryTypeVector output_memory_types;
        TF_RETURN_IF_ERROR(MemoryTypesForNode(
            OpRegistry::Global(), DeviceType(flr->device()->device_type()), nodeDefPerIsa,
            &input_memory_types, &output_memory_types));

        OpKernelConstruction construction(
            DeviceType(flr->device()->device_type()),
            flr->device(), flr->device()->GetAllocator(AllocatorAttributes()),
            &nodeDefPerIsa,
            &fbody->fdef.signature(), flr,
            fbody->arg_types, input_memory_types,
            fbody->ret_types, output_memory_types,
            flr->graph_def_version(), &s);
        kernel->reset(new TPerisaOp(handle, &construction));

        if (!s.ok()) {
            VLOG(1) << "Perisa: install no kernel for " << nodeDef.name() << " = " << nodeDef.op() << ", " << s.ToString();
            kernel->reset(nullptr);
        } else {
            VLOG(1) << "Perisa: install kernel for " << nodeDefPerIsa.DebugString();
        }
        return s;
    }
}
