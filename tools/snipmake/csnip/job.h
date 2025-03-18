#pragma once

#include <tools/snipmake/lib/job_processor/processor.h>
#include <tools/snipmake/metasnip/jobqueue/jobqueue.h>

#include <kernel/qtree/richrequest/richnode.h>
#include <kernel/relevfml/rank_models_factory.h>
#include <kernel/snippets/config/config.h>
#include <kernel/snippets/idl/snippets.pb.h>
#include <kernel/snippets/iface/configparams.h>
#include <kernel/snippets/iface/passagereply.h>
#include <kernel/tarc/docdescr/docdescr.h>
#include <library/cpp/stopwords/stopwords.h>

#include <util/datetime/base.h>
#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/memory/blob.h>

namespace NSnippets {

    struct IGeobase;
    struct TPoolMaker;
    struct TAssessorDataManager;
    class THostStats;
    class TAnswerModels;

    class TContextData : public IContextData {
    private:
        TString RawInput;
        TString Base64Data;
        TString Id;
        TString SubId;
        NSnippets::NProto::TSnippetsCtx ProtobufData;
        bool HasProto = false;
        bool HasBase64 = false;

    public:
        TContextData();
        void SetEmpty();
        void SetFromBase64Data(TStringBuf recId, TStringBuf recSubId, TStringBuf data);
        void SetFromProtobufData(TString recId, TString recSubId, const TString& rawInput, const NSnippets::NProto::TSnippetsCtx& protobufData);
        const TString& GetId() const;
        const TString& GetSubId() const;
        const TString& GetRawInput() const override;
        bool ParseToProtobuf();
        const NSnippets::NProto::TSnippetsCtx& GetProtobufData() const;
    };

    struct TJobEnv {
        const TAssessorDataManager& AssessorDataManager;
        TPoolMaker& PoolMaker;
        TWordFilter StopWords;
        THolder<TWordFilter> StopWordsExp;
        THolder<IRankModelsFactory> DynamicModels;
        const TAnswerModels* AnswerModels = nullptr;
        const THostStats* HostStats = nullptr;

        const NNeuralNetApplier::TModel* RuFactSnippetDssmApplier = nullptr;
        const NNeuralNetApplier::TModel* TomatoDssmApplier = nullptr;

        IGeobase* Geobase = nullptr;
        bool NeedAdjust = false;
        bool A3P = false;
        bool WithUnused = false;
        bool WithManual = false;
        TString InfoRequest;

        TJobEnv(TPoolMaker& poolMaker, const TAssessorDataManager& assessorDataManager);
        bool IsUpdate() const;
        void Adjust(bool a3p, bool wu, bool wm);
    };

    struct TJob : IObjectInQueue {
        TContextData ContextData;
        THolder<TContextData> ExpContextData;
        TStringBuf BaseExp;
        TStringBuf Exp;
        NSnippets::TJobReport Account;
        NSnippets::TPassageReply Reply;
        NSnippets::TPassageReply ReplyExp;
        TString Req;
        TString UserReq;
        TString B64QTree;
        TString ArcUrl;
        const TJobEnv& JobEnv;

        TRichTreeConstPtr Richtree;
        TRichTreeConstPtr MoreHlRichtree;
        TRichTreeConstPtr RegionPhraseRichtree;
        TString MoreHlReq;
        TString RegionReq;

        TBlob DocDBlob;
        TDocDescr DocD;
        TDocInfosPtr DocInfos;

        TString UIL;
        ui64 Region;

        TRankingFactors RankingFactors;

        /*
         * Works only on linux 2.6.26+ and freebsd 8.1+, where getrusage knows RUSAGE_THREAD.
         */
        TDuration UTime;

        TJob(
            const TContextData& contextData,
            const TContextData* expContextData,
            TStringBuf baseExp,
            TStringBuf exp,
            const TJobEnv& jobEnv);

        void FillReply(
            const NSnippets::NProto::TSnippetsCtx& ctx,
            NSnippets::TPassageReply& reply,
            const TStringBuf& exp,
            const TWordFilter& stopwords);

        void Process(void* thr = nullptr) override;
    };

    using IInputProcessor = ITInputProcessor<TContextData>;
    using IConverter = ITConverter<TContextData>;
    using TPairedInput = TTPairedInput<TContextData>;
    using IOutputProcessor = ITOutputProcessor<TJob>;


    bool Differs(const NSnippets::TPassageReply& a, const NSnippets::TPassageReply& b);
    bool DiffersInVisibleParts(const NSnippets::TPassageReply& a, const NSnippets::TPassageReply& b);

    TString GetRequestId(const TContextData& ctx);

} //namespace NSnippets
