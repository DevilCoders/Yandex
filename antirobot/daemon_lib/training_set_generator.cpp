#include "training_set_generator.h"

#include "config_global.h"
#include "request_context.h"
#include "unified_agent_log.h"

#include <antirobot/idl/antirobot.ev.pb.h>

#include <util/random/random.h>


namespace NAntiRobot {

namespace {

class TNullTrainingSetGenerator : public ITrainingSetGenerator
{
public:
    EResult ProcessRequest(const TRequestContext&) override {
        return RequestNotUsed;
    }
    TStringBuf Name() override {
        return "none"sv;
    }
};

class TRandomUserTrainingSetGenerator : public ITrainingSetGenerator
{
public:
    EResult ProcessRequest(const TRequestContext& rc) override {
        if (CanAddTrainingRequest(rc)) {
            return AddedRandomUserRequest;
        }
        return RequestNotUsed;
    }
    TStringBuf Name() override {
        return "random_factors"sv;
    }

private:
    bool CanAddTrainingRequest(const TRequestContext& rc) {
        const TRequest& req = *rc.Req;
        return req.CanShowCaptcha()
               && !rc.Req->UserAddr.IsPrivileged() // Для привилегированных запросов не считаются факторы и MatrixNet
               && RandomNumber<float>() < ANTIROBOT_DAEMON_CONFIG.JsonConfig.ParamsByTld(req.HostType, req.Tld).RandomFactorsProbability;
    }
};

}

ITrainingSetGenerator* NullTrainingSetGenerator() {
    return Singleton<TNullTrainingSetGenerator>();
}

ITrainingSetGenerator* RandomUserTrainingSetGenerator() {
    return Singleton<TRandomUserTrainingSetGenerator>();
}

} /* namespace NAntiRobot */
