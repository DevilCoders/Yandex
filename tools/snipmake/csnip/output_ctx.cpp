#include "output_ctx.h"

#include <kernel/snippets/idl/snippets.pb.h>

#include <library/cpp/string_utils/base64/base64.h>

namespace NSnippets {

    TCtxDiffOutput::TCtxDiffOutput(bool inverse, bool withId, bool needSubId)
      : Inverse(inverse)
      , WithId(withId)
      , NeedSubId(needSubId)
    {
    }

    void TCtxDiffOutput::Process(const TJob& job) {
        if (Inverse != !Differs(job.Reply, job.ReplyExp)) {
            return;
        }
        const TContextData& ctx = job.ContextData;
        if (WithId)
            Cout << ctx.GetId() << "\t";
        if (NeedSubId)
            Cout << ctx.GetSubId() << "\t";
        Cout << ctx.GetRawInput() << Endl;
    }


    void TCtxOutput::Process(const TJob& job) {
        Cout << job.ContextData.GetRawInput() << Endl;
    }


    void TUniqueCtxOutput::Process(const TJob& job) {
        Dict::iterator url = UrlToUsrReq.find(job.ArcUrl);
        if (url == UrlToUsrReq.end()) {
            UrlToUsrReq[job.ArcUrl].insert(job.UserReq);
        } else {
            std::pair<SetStrok::iterator,bool> ret =  url->second.insert(job.UserReq);
            if (ret.second == false)
                return;
        }
        Cout << job.ContextData.GetRawInput() << Endl;
    }


    void TICtxOutput::Process(const TJob& job) {
        Cout << job.ContextData.GetId() << '\t' << job.ContextData.GetRawInput() << Endl;
    }


    TPatchCtxOutput::TPatchCtxOutput(const TString& patch)
      : Patch(patch)
    {
    }

    void TPatchCtxOutput::Process(const TJob& job) {
        const TString buf = Base64Decode(Patch);
        NProto::TSnippetsCtx patchCtx;
        if (patchCtx.ParseFromArray(buf.data(), buf.size())) {
            NProto::TSnippetsCtx ctx = job.ContextData.GetProtobufData();
            ctx.MergeFrom(patchCtx);
            TString res;
            Y_PROTOBUF_SUPPRESS_NODISCARD ctx.SerializeToString(&res);
            Cout << Base64Encode(res) << Endl;
        }
        else {
            Cout << job.ContextData.GetRawInput() << Endl;
        }
    }

} //namespace NSnippets
