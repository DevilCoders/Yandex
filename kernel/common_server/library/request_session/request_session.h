#pragma once
#include <kernel/common_server/api/history/common.h>
#include <kernel/common_server/library/interfaces/container.h>
#include <kernel/common_server/library/interfaces/proto.h>
#include <kernel/common_server/library/logging/accumulator.h>
#include <kernel/common_server/library/request_session/proto/common.pb.h>
#include <kernel/common_server/library/scheme/abstract.h>
#include <kernel/common_server/util/accessor.h>
#include <kernel/common_server/util/network/http_request.h>
#include <kernel/common_server/util/network/neh_request.h>

class IBaseServer;

namespace NExternalAPI {
    class IRequestSession: public INativeProtoSerialization<NLogisticProto::TRequestSession> {
    private:
        virtual NFrontend::TScheme DoGetScheme(const IBaseServer& server) const = 0;
        virtual void DoSerializeToProto(NLogisticProto::TRequestSession& proto) const = 0;
        virtual bool DoDeserializeFromProto(const NLogisticProto::TRequestSession& proto) = 0;
        virtual void DoAppendProblemDetails(const TString& key, const TString& value) = 0;
        virtual void DoSetProblemDetails(const NCS::NLogging::TDefaultLogsAccumulator& accumulator) = 0;

    public:
        using TPtr = TAtomicSharedPtr<IRequestSession>;
        using TFactory = NObjectFactory::TObjectFactory<IRequestSession, TString>;
        virtual TString GetClassName() const = 0;
        virtual NFrontend::TScheme GetScheme(const IBaseServer& server) const final;
        virtual bool DeserializeFromProto(const NLogisticProto::TRequestSession& proto) override final;
        virtual void SerializeToProto(NLogisticProto::TRequestSession& proto) const override final;
        virtual void AppendProblemDetails(const TString& key, const TString& value) final;
        virtual void SetProblemDetails(const NCS::NLogging::TDefaultLogsAccumulator& accumulator) final;
    };

    class TRequestSessionContainer: public TInterfaceContainer<IRequestSession> {
    private:
        using TBase = TInterfaceContainer<IRequestSession>;

    protected:
        using TBase::Object;

    public:
        using TBase::TBase;
    };

    class THTTPRequestSession: public IRequestSession {
        CSA_MAYBE(THTTPRequestSession, NJson::TJsonValue, Request);
        CSA_MAYBE(THTTPRequestSession, NJson::TJsonValue, Response);
        CSA_READONLY_MAYBE(NJson::TJsonValue, ProblemDetails);

    private:
        static TFactory::TRegistrator<THTTPRequestSession> Registrator;
        virtual NFrontend::TScheme DoGetScheme(const IBaseServer& server) const override;
        virtual void DoSerializeToProto(NLogisticProto::TRequestSession& proto) const override;
        virtual bool DoDeserializeFromProto(const NLogisticProto::TRequestSession& proto) override;
        virtual void DoAppendProblemDetails(const TString& key, const TString& value) override;
        virtual void DoSetProblemDetails(const NCS::NLogging::TDefaultLogsAccumulator& accumulator) override;

    public:
        THTTPRequestSession() = default;

        static TAtomicSharedPtr<THTTPRequestSession> BuildFailed(const NCS::NLogging::TDefaultLogsAccumulator& accumulator);
        static TAtomicSharedPtr<THTTPRequestSession> BuildFailed(const TString& reason);
        static TAtomicSharedPtr<THTTPRequestSession> BuildFromRequestResponse(const NNeh::THttpRequest& request, const NUtil::THttpReply& response);
        THTTPRequestSession& SetRequest(const NNeh::THttpRequest request);
        THTTPRequestSession& SetResponse(const NUtil::THttpReply& response);
        THTTPRequestSession& SetRequest(const TString& requestString);
        THTTPRequestSession& SetResponse(const TString& responseString);

        static TString GetTypeName(){
            return "http_request_session";
        }

        public: virtual TString GetClassName() const override {
            return GetTypeName();
        }
    };

    class TRequestDialog {
        using TRequestSessions = TVector<TRequestSessionContainer>;
        CSA_DEFAULT(TRequestDialog, TRequestSessions, RequestSessions);

    public:
        static NFrontend::TScheme GetScheme(const IBaseServer& server);
        bool DeserializeFromProto(const NLogisticProto::TRequestDialog& proto);
        NLogisticProto::TRequestDialog SerializeToProto() const;

        void AddRequestSession(TRequestSessionContainer&& requestSession) {
            RequestSessions.emplace_back(std::move(requestSession));
        }

        void AddRequestSession(const TRequestSessionContainer& requestSession) {
            RequestSessions.emplace_back(requestSession);
        }

        void AddRequestDialog(const TRequestDialog& requestDialog) {
            for (auto&& i : requestDialog) {
                RequestSessions.emplace_back(i);
            }
        }

        size_t size() const {
            return RequestSessions.size();
        }

        bool empty() const {
            return RequestSessions.empty();
        }

        TVector<TRequestSessionContainer>::const_iterator begin() const {
            return RequestSessions.begin();
        }

        TVector<TRequestSessionContainer>::const_iterator end() const {
            return RequestSessions.end();
        }
    };
}
