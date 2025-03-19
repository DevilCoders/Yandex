#include "replier.h"
#include <search/session/reqenv.h>
#include <search/session/comsearch.h>

TCommonSearchReplier::TCommonSearchReplier(IReplyContext::TPtr context, const TCommonSearch* commonSearcher, const THttpStatusManagerConfig* httpStatusConfig)
    : ISearchReplier(context, httpStatusConfig)
    , CommonSearcher(commonSearcher)
    , LogFrame(nullptr)
    , StatFrame(nullptr)
    , RequestType(RT_Unknown)
{
    VERIFY_WITH_LOG(!!CommonSearcher, "CommonSearcher can't be nullptr");
    RequestType = CommonSearcher->RequestType(Context->GetCgiParameters());
}

void TCommonSearchReplier::CreateLogFrame() {
    VERIFY_WITH_LOG(!!CommonSearcher, "CommonSearcher can't be nullptr");
    if (!LogFrame) {
        LogFrame = TSelfFlushLogFramePtr(MakeHolder<TSelfFlushLogFrame>(*CommonSearcher->GetEventLog()).Release());
    }
    const size_t queueSize = 0;
    const NEvClass::TEnqueueYSRequest::QType qtype = NEvClass::TEnqueueYSRequest::SEARCH;
    TStringBuf addr = Context->GetRequestData().RemoteAddr();

    LogFrame->LogEvent(NEvClass::TEnqueueYSRequest(Context->GetRequestId(), qtype, queueSize, TString{Context->GetRequestData().ScriptName()}));
    LogFrame->LogEvent(NEvClass::TPeerName(!addr.empty() ? TString{addr} : "no addr"));
}

IThreadPool* TCommonSearchReplier::DoSelectHandler() {
    CreateLogFrame();
    return GetHandler(RequestType);
}

const TSearchHandlers* TCommonSearchReplier::GetSearchHandlers() const {
    return CommonSearcher;
}

void TCommonSearchReplier::DoSearchAndReply() {
    switch (RequestType) {
    case RT_Fetch:
        ProcessFetch();
        break;
    case RT_Info:
        if (!CommonSearcher->InfoRequestRequiresPseudoSearch(Context->MutableRequestData())) {
            ProcessInfo();
            break;
        } else {
            // fallthrough to search
            [[fallthrough]];
        }
    default:
        ProcessRequest();
        break;
    }
}

TMakePageContext TCommonSearchReplier::CreateMakePageContext() {
    VERIFY_WITH_LOG(!!CommonSearcher, "CommonSearcher can't be nullptr");
    if (!LogFrame) {
        LogFrame = TSelfFlushLogFramePtr(MakeIntrusive<TSelfFlushLogFrame>(*CommonSearcher->GetEventLog()));
    }
    StatFrame = TUnistatFramePtr(MakeIntrusive<TUnistatFrame>());

    TMakePageContext context = {
        CommonSearcher,
        &Context->MutableRequestData(),
        nullptr,
        {
            &Context->Output(),
            nullptr,
            LogFrame.Get(),
            StatFrame.Get()
        },
        Context->GetBuf(),
        (ui32)Context->GetRequestId(),
        RequestType,
        !Context->IsHttp(),
        this,
        GetRUsageCheckPoint(),
    };
    return context;
}

IThreadPool* ISearchReplier::GetHandler(ERequestType requestType) {
    const TSearchHandlers* handlers = GetSearchHandlers();
    if (!handlers) {
        return nullptr;
    }

    switch (requestType) {
        case RT_Fetch:
            return handlers->PassagesHandler.Get();
        case RT_Factors:
            return handlers->FactorsHandler.Get();
        case RT_Info:
            return handlers->InfoHandler.Get();
        case RT_DbgSearch:
            return handlers->DbgReqsHandler.Get();
        case RT_Reask:
            return handlers->ReAskHandler.Get();
        case RT_LongSearch:
            return handlers->LongReqsHandler.Get();
        case RT_PreSearch:
            return handlers->PreSearchReqsHandler.Get();
        default:
            break;
    }
    return handlers->MainHandler.Get();
}

bool TCommonSearchReplier::ProcessRequest()
{
    return (MakePage(CreateMakePageContext()) == YXOK);
}

bool TCommonSearchReplier::ProcessFetch()
{
    return (MakePassages(CreateMakePageContext()) == YXOK);
}

bool TCommonSearchReplier::ProcessInfo()
{
    return (MakeInfo(CreateMakePageContext()) == YXOK);
}

int TCommonSearchReplier::BeforeReport(ISearchContext* ctx) const {
    Stat.TotalDocCount = ctx->Cluster()->TotalDocCount(0);
    Stat.UnanswerCount = ctx->ReqEnv()->BaseSearchNotRespondCount();
    Stat.CacheHit = static_cast<TReqEnv*>(ctx->ReqEnv())->CacheHit();
    return 0;
}
