#include "ctx.h"

#include <kernel/snippets/idl/snippets.pb.h>

#include <util/generic/algorithm.h>
#include <util/ysaveload.h>
#include <util/stream/input.h>
#include <util/stream/output.h>

namespace NSnippets
{
    void THitsInfo::Load(const NProto::THitsCtx& res)
    {
        Kind = res.HasKind() ? THitsKind::Type(res.GetKind()) : THitsKind::Nothing;
        TextSentsPlain.assign(res.GetTextHitsPlain().begin(), res.GetTextHitsPlain().end());
        MoreTextSentsPlain.assign(res.GetMoreTextHitsPlain().begin(), res.GetMoreTextHitsPlain().end());
        THSents.assign(res.GetTHSents().begin(), res.GetTHSents().end());
        THMasks.assign(res.GetTHMasks().begin(), res.GetTHMasks().end());
        FormWeights.assign(res.GetFormWeights().begin(), res.GetFormWeights().end());
        LinkHits.assign(res.GetLinkHitsPlain().begin(), res.GetLinkHitsPlain().end());
        IsNav = res.HasIsNav() && res.GetIsNav();
        IsRoot = res.HasIsRoot() && res.GetIsRoot();
        DocStaticSig = res.HasDocStaticSig() ? res.GetDocStaticSig() : 0;
        WatchVideo = res.HasWatchVideo() && res.GetWatchVideo();
        IsTurkey = res.HasIsTurkey() && res.GetIsTurkey();
        IsSpok = res.HasIsSpok() && res.GetIsSpok();
        IsFakeForBan = res.HasIsFakeForBan() && res.GetIsFakeForBan();
        IsFakeForRedirect = res.HasIsFakeForRedirect() && res.GetIsFakeForRedirect();
    }

    void THitsInfo::Save(NProto::THitsCtx& res) const
    {
        res.SetKind(ui64(Kind));
        Copy(TextSentsPlain.begin(), TextSentsPlain.end(), google::protobuf::RepeatedFieldBackInserter(res.MutableTextHitsPlain()));
        Copy(MoreTextSentsPlain.begin(), MoreTextSentsPlain.end(), google::protobuf::RepeatedFieldBackInserter(res.MutableMoreTextHitsPlain()));
        Copy(THSents.begin(), THSents.end(), google::protobuf::RepeatedFieldBackInserter(res.MutableTHSents()));
        Copy(THMasks.begin(), THMasks.end(), google::protobuf::RepeatedFieldBackInserter(res.MutableTHMasks()));
        Copy(FormWeights.begin(), FormWeights.end(), google::protobuf::RepeatedFieldBackInserter(res.MutableFormWeights()));
        Copy(LinkHits.begin(), LinkHits.end(), google::protobuf::RepeatedFieldBackInserter(res.MutableLinkHitsPlain()));
        res.SetIsNav(IsNav);
        res.SetIsRoot(IsRoot);
        res.SetDocStaticSig(DocStaticSig);
        res.SetWatchVideo(WatchVideo);
        res.SetIsTurkey(IsTurkey);
        res.SetIsSpok(IsSpok);
        res.SetIsFakeForBan(IsFakeForBan);
        res.SetIsFakeForRedirect(IsFakeForRedirect);
    }

    void THitsInfo::Load(IInputStream& str)
    {
        NProto::THitsCtx tmp;
        if (tmp.ParseFromArcadiaStream(&str)) {
            Load(tmp);
        } else {
            ythrow yexception() << "can't parse protobuf message with hit's";
        }
    }

    void THitsInfo::Save(IOutputStream& str) const
    {
        NProto::THitsCtx tmp;
        Save(tmp);
        tmp.SerializeToArcadiaStream(&str);
    }

    TString THitsInfo::DebugString() const
    {
        NProto::THitsCtx tmp;
        Save(tmp);
        return tmp.DebugString();
    }
}

