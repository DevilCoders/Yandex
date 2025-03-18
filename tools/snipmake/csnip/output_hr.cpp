#include "output_hr.h"

#include <kernel/snippets/iface/passagereply.h>
#include <kernel/snippets/idl/snippets.pb.h>

#include <search/idl/meta.pb.h>

#include <google/protobuf/text_format.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>

#include <util/generic/string.h>
#include <util/string/cast.h>

namespace NSnippets {

    TProtobufOutput::TProtobufOutput(IOutputStream& out)
        : Out(out)
    {
    }

    void TProtobufOutput::Process(const TJob& job) {
        NMetaProtocol::TArchiveInfo ai;
        job.Reply.PackToArchiveInfo(ai);
        TString res;
        google::protobuf::io::StringOutputStream out(&res);
        google::protobuf::TextFormat::Print(ai, &out);
        Out << res << Endl;
        Out.Flush();
    }


    void TSrCtxOutput::Process(const TJob& job) {
        const NProto::TSnippetsCtx& ctx = job.ContextData.GetProtobufData();
        TString res;
        google::protobuf::io::StringOutputStream out(&res);
        google::protobuf::TextFormat::Print(ctx, &out);
        Cout << res << Endl;
    }


    static void FixData(TString* data) {
        *data = TString("Blob of ") + ToString(data->size()) + " bytes";
    }

    void THrCtxOutput::Process(const TJob& job) {
        NProto::TSnippetsCtx ctx = job.ContextData.GetProtobufData();
        if (ctx.HasTextArc() && ctx.GetTextArc().HasData()) {
            FixData(ctx.MutableTextArc()->MutableData());
        }
        if (ctx.HasLinkArc() && ctx.GetLinkArc().HasData()) {
            FixData(ctx.MutableLinkArc()->MutableData());
        }
        TString res;
        google::protobuf::io::StringOutputStream out(&res);
        google::protobuf::TextFormat::Print(ctx, &out);
        Cout << res << Endl;
    }

}
