#pragma once
#include <kernel/common_server/util/auto_actualization.h>
#include <kernel/common_server/library/searchserver/simple/context/replier.h>
#include <kernel/common_server/util/network/http_request.h>
#include "case.h"

namespace NCS {

    class IEmulationHttpCase: public IEmulationCase {
    public:
        virtual bool CheckHttpRequest(const IReplyContext& req) const = 0;
        virtual NUtil::THttpReply GetHttpReply(const IReplyContext& req) const = 0;
    };

    class THttpHeaderEmulation {
    private:
        CSA_DEFAULT(THttpHeaderEmulation, TString, Name);
        CSA_DEFAULT(THttpHeaderEmulation, TString, Value);
    public:
        NJson::TJsonValue SerializeToJson() const;

        Y_WARN_UNUSED_RESULT bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo);
    };

    class TEmulationHttpCase: public IEmulationHttpCase {
    private:
        static TFactory::TRegistrator<TEmulationHttpCase> Registrator;
        CSA_DEFAULT(TEmulationHttpCase, TSet<TString>, Uri);
        CSA_DEFAULT(TEmulationHttpCase, TString, ReplyBody);
        CSA_DEFAULT(TEmulationHttpCase, ui32, ReplyCode);
        CS_ACCESS(TEmulationHttpCase, TString, ContentType, "text/plain");
        CSA_DEFAULT(TEmulationHttpCase, TVector<THttpHeaderEmulation>, ReplyHeaders);
    protected:
        virtual NJson::TJsonValue DoSerializeToJson() const override;
        virtual bool DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) override;
    public:
        virtual TString GetClassName() const override {
            return GetTypeName();
        }

        virtual bool CheckHttpRequest(const IReplyContext& context) const override;
        virtual NUtil::THttpReply GetHttpReply(const IReplyContext& /*context*/) const override;
        static TString GetTypeName() {
            return "http";
        }
    };

}
