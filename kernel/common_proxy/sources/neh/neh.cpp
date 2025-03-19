#include "neh.h"
#include <library/cpp/logger/global/global.h>
#include <library/cpp/http/misc/httpcodes.h>

namespace NCommonProxy {

    class TNehSource::TOutMetaData : public TMetaData {
    public:
        TOutMetaData() {
            Register("cgi", dtSTRING);
            Register("post", dtBLOB);
            Register("uri", dtSTRING);
        }
    };


    void TNehSource::ServeRequest(const NNeh::IRequestRef& request) {
        NNeh::IRequest* req = request.Get();
        IReplier::TPtr replier;
        if (Config.GetInstantReply()) {
            replier = MakeIntrusive<TSource::TReplier>(*this);
        } else {
            replier = MakeIntrusive<TReplier>(request, *this);
        }

        try {
            TDataSet::TPtr data = MakeIntrusive<TDataSet>(GetOutputMetaData());
            data->Set<TString>("cgi", "");
            TStringBuf post = req->Data();
            TStringBuf serv = req->Service();
            data->Set<TString>("uri", TString(serv.data(), serv.size()));
            if (Config.GetInstantReply()) {
                data->Set<TBlob>("post", TBlob::Copy(post.data(), post.size()));
                NNeh::TDataSaver reply;
                reply << "200:";
                req->SendReply(reply);
            } else {
                data->Set<TBlob>("post", TBlob::NoCopy(post.data(), post.size()));
            }
            Process(data, replier);
        } catch (...) {
            replier->AddReply(GetName(), 500, CurrentExceptionMessage());
        }
    }

    TNehSource::TNehSource(const TString& name, const TProcessorsConfigs& configs)
        : TSource(name, configs)
        , Config(*configs.Get<TConfig>(name))
        , Loop(NNeh::CreateLoop())
    {
        for (auto&& addr : Config.GetListenAdresses()) {
            Loop->Add(addr, *this);
        }
    }

    void TNehSource::Run() {
        Loop->ForkLoop(Config.GetThreads());
    }

    void TNehSource::DoStop() {
        Loop->Stop();
    }

    void TNehSource::DoWait() {
        Loop->SyncStopFork();
    }

    const NCommonProxy::TMetaData& TNehSource::GetOutputMetaData() const {
        return Default<TOutMetaData>();
    }

    TNehSource::TReplier::TReplier(const NNeh::IRequestRef& request, const TSource& source)
        : TSource::TReplier(source, new TReporter(*this, request.Get()))
        , Request(*request.Release())
    {}


    bool TNehSource::TReplier::Canceled() const {
        return Request.Canceled();
    }

    TNehSource::TReplier::TReporter::TReporter(TSource::TReplier& owner, NNeh::IRequest* request)
        : TSource::TReplier::TReporter(owner)
        , Request(request)
    {}

    void TNehSource::TReplier::TReporter::Report(const TString& message) {
        NNeh::TDataSaver data;
        data << Owner.GetCode() << ":" << message;
        switch (Owner.GetCode()) {
        case HttpCodes::HTTP_ACCEPTED:
        case HttpCodes::HTTP_OK:
            break;
        case HttpCodes::HTTP_BAD_REQUEST:
            Request->SendError(NNeh::IRequest::TResponseError::BadRequest, message);
            break;
        case HttpCodes::HTTP_FORBIDDEN:
            Request->SendError(NNeh::IRequest::TResponseError::Forbidden, message);
            break;
        case HttpCodes::HTTP_NOT_FOUND:
            Request->SendError(NNeh::IRequest::TResponseError::NotExistService, message);
            break;
        case HttpCodes::HTTP_TOO_MANY_REQUESTS:
            Request->SendError(NNeh::IRequest::TResponseError::TooManyRequests, message);
            break;
        case HttpCodes::HTTP_INTERNAL_SERVER_ERROR:
            Request->SendError(NNeh::IRequest::TResponseError::InternalError, message);
            break;
        case HttpCodes::HTTP_NOT_IMPLEMENTED:
            Request->SendError(NNeh::IRequest::TResponseError::NotImplemented, message);
            break;
        case HttpCodes::HTTP_BAD_GATEWAY:
            Request->SendError(NNeh::IRequest::TResponseError::BadGateway, message);
            break;
        case HttpCodes::HTTP_SERVICE_UNAVAILABLE:
            Request->SendError(NNeh::IRequest::TResponseError::ServiceUnavailable, message);
            break;
        case HttpCodes::HTTP_BANDWIDTH_LIMIT_EXCEEDED:
            Request->SendError(NNeh::IRequest::TResponseError::BandwidthLimitExceeded, message);
            break;
        default:
            Request->SendError(NNeh::IRequest::TResponseError::InternalError, message);
            break;
        }
        Request->SendReply(data);
        TSource::TReplier::TReporter::Report(message);
    }

    bool TNehSource::TConfig::DoCheck() const {
        if (ListenAddresses.empty()) {
            ERROR_LOG << "There is no any listen addreses set." << Endl;
            return false;
        }
        return true;
    }

    void TNehSource::TConfig::DoInit(const TYandexConfig::Section& componentSection) {
        TSource::TConfig::DoInit(componentSection);
        TYandexConfig::TSectionsMap sm = componentSection.GetAllChildren();
        componentSection.GetDirectives().GetValue("InstantReply", InstantReply);
        for (auto range = sm.equal_range("Listen"); range.first != range.second; ++range.first) {
            if (TString addr = range.first->second->GetDirectives().Value("Address", TString())) {
                ListenAddresses.emplace_back(std::move(addr));
            }
        }
    }

    void TNehSource::TConfig::DoToString(IOutputStream& so) const {
        TSource::TConfig::DoToString(so);
        so << "InstantReply: " << InstantReply << Endl;
        for (auto&& addr : ListenAddresses) {
            so << "<Listen>" << Endl;
            so << "Address: " << addr << Endl;
            so << "</Listen>" << Endl;
        }
    }

}
