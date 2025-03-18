#include "job.h"
#include "pool_utils.h"
#include "inforeq.h"

#include <extsearch/images/base/snippets/snippetizer/snippetizer_registrar.h>
#include <extsearch/video/base/snippets/snippetizer.h>

#include <tools/snipmake/lib/job_processor/thrusage.h>

#include <kernel/snippets/base/default_snippetizer.h>
#include <kernel/snippets/hits/ctx.h>
#include <kernel/snippets/iface/archive/manip.h>
#include <kernel/snippets/iface/passagereplyfiller.h>
#include <kernel/snippets/iface/replycontext.h>
#include <kernel/snippets/strhl/goodwrds.h>
#include <kernel/snippets/urlcut/url_wanderer.h>

#include <kernel/qtree/richrequest/printrichnode.h>
//#include <library/cpp/digest/sfh/sfh.h>

#include <util/charset/wide.h>
#include <util/generic/ylimits.h>
#include <util/stream/output.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <util/string/cast.h>

REGISTER_PASSAGE_REPLY_FILLER_FACTORY(images, ::NSnippets::EPassageReplyFillerType::PRFT_IMAGES, new NImages::NSnippetizer::TImagesPassageReplyFillerFactory)
REGISTER_PASSAGE_REPLY_FILLER_FACTORY(Video, NSnippets::PRFT_VIDEO, NVideoSnippets::CreateSnippetizerFactory())

namespace NSnippets {

    TContextData::TContextData() {
        SetEmpty();
    }

    void TContextData::SetEmpty() {
        RawInput.clear();
        Base64Data.clear();
        Id.clear();
        ProtobufData.Clear();
        HasProto = false;
        HasBase64 = false;
    }

    void TContextData::SetFromBase64Data(TStringBuf recId, TStringBuf recSubId, TStringBuf data) {
        RawInput.assign(data);
        Base64Data = RawInput;
        Id.assign(recId);
        SubId.assign(recSubId);
        ProtobufData.Clear();
        HasProto = false;
        HasBase64 = true;
    }

    void TContextData::SetFromProtobufData(TString recId, TString recSubId, const TString& rawInput, const NSnippets::NProto::TSnippetsCtx& protobufData) {
        RawInput = rawInput;
        Base64Data.clear();
        Id = recId;
        SubId = recSubId;
        ProtobufData = protobufData;
        HasProto = true;
        HasBase64 = false;
    }

    const TString& TContextData::GetId() const {
        return Id;
    }

    const TString& TContextData::GetSubId() const {
        return SubId;
    }

    const TString& TContextData::GetRawInput() const {
        return RawInput;
    }

    bool TContextData::ParseToProtobuf() {
        if (HasProto) {
            return true;
        }
        if (!HasBase64 || Base64Data.size() % 4 != 0) {
            return false;
        }
        const TString buf = Base64Decode(Base64Data);
        HasProto = ProtobufData.ParseFromArray(buf.data(), buf.size());
        return HasProto;
    }

    const NSnippets::NProto::TSnippetsCtx& TContextData::GetProtobufData() const {
        Y_ASSERT(HasProto);
        return ProtobufData;
    }

    static TString GetDumpExp(bool a3p, bool wu, bool wm, bool pool) {
        TString res;
        if (a3p) {
            res += "dumpall,";
        }
        if (wu) {
            res += "dumpu,";
        }
        if (wm) {
            res += "dumpm,";
        }
        if (pool) {
            res += "dumpforpool,";
        }
        return res;
    }

    TJobEnv::TJobEnv(TPoolMaker& poolMaker, const TAssessorDataManager& assessorDataManager)
        : AssessorDataManager(assessorDataManager)
        , PoolMaker(poolMaker)
    {
    }

    bool TJobEnv::IsUpdate() const
    {
        return !AssessorDataManager.IsEmpty();
    }

    void TJobEnv::Adjust(bool a3p, bool wu, bool wm)
    {
        NeedAdjust = true;
        A3P = a3p;
        WithUnused = wu;
        WithManual = wm;
    }

    TJob::TJob(const TContextData& contextData, const TContextData* expContextData, TStringBuf baseExp, TStringBuf exp, const TJobEnv& jobEnv)
      : ContextData(contextData)
      , ExpContextData(expContextData ? new TContextData(*expContextData) : nullptr)
      , BaseExp(baseExp)
      , Exp(exp)
      , Account()
      , JobEnv(jobEnv)
      , Richtree()
      , DocDBlob()
      , DocD()
      , DocInfos()
      , UIL()
      , Region()
      , UTime()
    {
    }

    void TJob::FillReply(const NSnippets::NProto::TSnippetsCtx& ctx, NSnippets::TPassageReply& reply, const TStringBuf& exp, const TWordFilter& stopwords) {
        reply.Reset();

        NSnippets::THitsInfoPtr hitsInfo;
        if (ctx.HasHits()) {
            hitsInfo = new NSnippets::THitsInfo();
            hitsInfo->Load(ctx.GetHits());
        }
        NSnippets::TVoidFetcher fetchText;
        NSnippets::TVoidFetcher fetchLink;
        NSnippets::TArcManip arcCtx(fetchText, fetchLink);
        if (ctx.HasTextArc()) {
            arcCtx.GetTextArc().LoadState(ctx.GetTextArc());
        }
        if (ctx.HasLinkArc()) {
            arcCtx.GetLinkArc().LoadState(ctx.GetLinkArc());
        }
        if (ctx.HasQuery()) {
            if (ctx.GetQuery().HasQtreeBase64()) {
                B64QTree = ctx.GetQuery().GetQtreeBase64();
                Richtree = DeserializeRichTree(DecodeRichTreeBase64(B64QTree));
                Req = WideToUTF8(PrintRichRequest(*Richtree->Root.Get()));
            }
            if (ctx.GetQuery().HasUserRequest()) {
                UserReq = ctx.GetQuery().GetUserRequest();
            } else {
                UserReq = Req;
            }
            if (ctx.GetQuery().HasMoreHlTreeBase64()) {
                MoreHlRichtree = DeserializeRichTree(DecodeRichTreeBase64(ctx.GetQuery().GetMoreHlTreeBase64()));
                MoreHlReq = WideToUTF8(PrintRichRequest(*MoreHlRichtree->Root.Get()));
            }
            if (ctx.GetQuery().HasRegionPhraseTreeBase64()) {
                RegionPhraseRichtree = DeserializeRichTree(DecodeRichTreeBase64(ctx.GetQuery().GetRegionPhraseTreeBase64()));
                RegionReq = WideToUTF8(PrintRichRequest(*RegionPhraseRichtree->Root.Get()));
            }
        }

        ui64 relevRegion = 0;
        ui64 userRegion = 0;
        bool ignoreRegionPhrase = false;
        if (ctx.HasReqParams()) {
            const NProto::TSnipReqParams& rp = ctx.GetReqParams();
            if (rp.HasRelevRegion()) {
                relevRegion = rp.GetRelevRegion();
            }
            if (rp.HasIgnoreRegionPhrase()) {
                ignoreRegionPhrase = rp.GetIgnoreRegionPhrase();
            }
            if (rp.HasUILang()) {
                UIL = rp.GetUILang();
            }
            RankingFactors.Load(rp);
        }
        if (ctx.HasLog() && ctx.GetLog().HasRegionId()) {
            userRegion = ctx.GetLog().GetRegionId();
        }
        if (userRegion != 0) {
            Region = userRegion;
        } else if (relevRegion != 0 && relevRegion != Max<ui64>()) {
            Region = relevRegion;
        } else {
            Region = 213;
        }

        DocDBlob = arcCtx.GetTextArc().GetDescrBlob(); // ref to persist
        DocD = arcCtx.GetTextArc().GetDescr();
        DocInfos = arcCtx.GetTextArc().GetDocInfosPtr();
        if (DocD.IsAvailable()) {
            ArcUrl = DocD.get_url();
        }
        if (JobEnv.PoolMaker.Skip(UserReq, ArcUrl)) {
            return;
        }

        NSnippets::TConfigParams cfgp;
        NSnippets::NProto::TSnipReqParams pbReqParams;
        if (ctx.HasReqParams()) {
            if (JobEnv.InfoRequest) {
                pbReqParams.CopyFrom(ctx.GetReqParams());
                SetInfoRequestParams(JobEnv.InfoRequest, pbReqParams);
                cfgp.SRP = &pbReqParams;
            } else {
                cfgp.SRP = &ctx.GetReqParams();
            }
        }
        if (JobEnv.NeedAdjust) {
            cfgp.AppendExps.push_back(GetDumpExp(JobEnv.A3P, JobEnv.WithUnused, JobEnv.WithManual, JobEnv.PoolMaker.PoolGeneration));
        }
        cfgp.StopWords = &stopwords;
        if (JobEnv.Geobase) {
            cfgp.Geobase = JobEnv.Geobase;
        }
        if (JobEnv.AnswerModels) {
            cfgp.AnswerModels = JobEnv.AnswerModels;
        }
        if (JobEnv.HostStats) {
            cfgp.HostStats = JobEnv.HostStats;
        }
        if (JobEnv.RuFactSnippetDssmApplier) {
            cfgp.RuFactSnippetDssmApplier = JobEnv.RuFactSnippetDssmApplier;
        }
        if (JobEnv.TomatoDssmApplier) {
            cfgp.TomatoDssmApplier = JobEnv.TomatoDssmApplier;
        }
        if (JobEnv.DynamicModels) {
            cfgp.DynamicModels = JobEnv.DynamicModels.Get();
        }
        cfgp.AppendExps.push_back(TString(exp));
        if (JobEnv.IsUpdate()) {
            TString retextExp = JobEnv.AssessorDataManager.GetRetextExp(UserReq, ArcUrl);
            if (retextExp.empty()) {
                return;
            }
            cfgp.AppendExps.push_back(retextExp);
        }

        TInlineHighlighter ih;
        if (Richtree.Get() && Richtree->Root.Get()) {
            ih.AddRequest(*Richtree->Root);
            if (MoreHlRichtree.Get() && MoreHlRichtree->Root.Get()) {
                ih.AddRequest(*MoreHlRichtree->Root, nullptr, true);
            }
            if (!ignoreRegionPhrase && RegionPhraseRichtree.Get() && RegionPhraseRichtree->Root.Get()) {
                ih.AddRequest(*RegionPhraseRichtree->Root, nullptr, true);
            }
        }

        NUrlCutter::TRichTreeWanderer wanderer(Richtree);
        const TPassageReplyContext args(cfgp, hitsInfo, ih, arcCtx, Richtree, RegionPhraseRichtree, wanderer);

        TAutoPtr<NSnippets::IPassageReplyFiller> passageReplyFiller = CreatePassageReplyFiller(args);
        passageReplyFiller->FillPassageReply(reply);
    }

    void TJob::Process(void* thr) {
        NSnippets::TReportGuard g(&Account);
        TThreadData* log = (TThreadData*)thr;
        log->Log(ContextData);

        if (!ContextData.ParseToProtobuf()) {
            Cerr << "Job " << ContextData.GetId() << ": corrupted context" << Endl;
            return;
        }
        if (ExpContextData.Get() && !ExpContextData->ParseToProtobuf()) {
            Cerr << "Job " << ExpContextData->GetId() << ": corrupted exp context" << Endl;
            return;
        }
        TThreadProfileTimer prof;
        FillReply(ContextData.GetProtobufData(), Reply, BaseExp, JobEnv.StopWords);
        if (Exp.size() || ExpContextData.Get() || JobEnv.StopWordsExp.Get()) {
            FillReply((ExpContextData.Get() ? ExpContextData.Get() : &ContextData)->GetProtobufData(), ReplyExp, Exp, JobEnv.StopWordsExp.Get() ? *JobEnv.StopWordsExp.Get() : JobEnv.StopWords);
        }
        UTime = prof.Get();
    }


    bool Differs(const NSnippets::TPassageReply& a, const NSnippets::TPassageReply& b) {
        return a.GetTitle() != b.GetTitle() || a.GetHeadline() != b.GetHeadline() || a.GetPassages() != b.GetPassages()
            || a.GetHeadlineSrc() != b.GetHeadlineSrc()
            || a.GetImagesJson() != b.GetImagesJson() || a.GetImagesDups() != b.GetImagesDups() || a.GetPreviewJson() != b.GetPreviewJson()
            || a.GetSpecSnippetAttrs() != b.GetSpecSnippetAttrs() || a.GetUrlMenu() != b.GetUrlMenu()
            || a.GetHilitedUrl() != b.GetHilitedUrl() || a.GetClickLikeSnip() != b.GetClickLikeSnip();
    }

    bool DiffersInVisibleParts(const NSnippets::TPassageReply& a, const NSnippets::TPassageReply& b) {
        return a.GetTitle() != b.GetTitle()
            || a.GetHilitedUrl() != b.GetHilitedUrl()
            || a.GetHeadline() != b.GetHeadline()
            || a.GetPassages() != b.GetPassages();
    }


    TString GetRequestId(const TContextData& ctx) {
        TString key;
        if (ctx.GetProtobufData().HasLog() && ctx.GetProtobufData().GetLog().HasRequestId())
            key = ctx.GetProtobufData().GetLog().GetRequestId();
        if (key.empty())
            key = ctx.GetId();
        return key;
    }

} //namespace NSnippets
