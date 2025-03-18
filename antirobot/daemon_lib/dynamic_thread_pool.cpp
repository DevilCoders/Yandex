#include "dynamic_thread_pool.h"

#include "config_global.h"

#include <library/cpp/http/cookies/cookies.h>

#include <util/generic/singleton.h>

namespace NAntiRobot {

namespace {

class TPoolCreationHelper {
public:
    TPoolCreationHelper()
    : Pool(ParseParams())
    {
    }

    TDynamicThreadPool* GetPool() {
        return &Pool;
    }
private:
    static TDynamicThreadPool::TParams ParseParams() {
        try {
            const THttpCookies params(ANTIROBOT_DAEMON_CONFIG.ThreadPoolParams);
            const TDynamicThreadPool::TParams poolParams = {
                TDynamicThreadPool::TParams::ParseValue(FromCookie<TString>(params, "free_min")),
                TDynamicThreadPool::TParams::ParseValue(FromCookie<TString>(params, "free_max")),
                TDynamicThreadPool::TParams::ParseValue(FromCookie<TString>(params, "total_max")),
                TDynamicThreadPool::TParams::ParseValue(FromCookie<TString>(params, "increase")),
            };
            return poolParams;
        } catch (const yexception& e) {
            ythrow yexception() << "AntiRobotDynamicThreadPool exception: "  << e.what();
        }
    }
private:
    TDynamicThreadPool Pool;
};

} // namespace unnamed

TDynamicThreadPool* GetAntiRobotDynamicThreadPool() {
    return Singleton<TPoolCreationHelper>()->GetPool();
}

} // namespace NAntiRobot
