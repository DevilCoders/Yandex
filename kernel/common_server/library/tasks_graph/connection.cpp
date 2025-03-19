#include "connection.h"
#include <library/cpp/logger/global/global.h>

namespace NRTYScript {

    void TCheckerContainer::Deserialize(const NJson::TJsonValue& info) {
        Checker = IConnectionChecker::TFactory::Construct(info["class"].GetStringRobust());
        CHECK_WITH_LOG(!!Checker);
        Checker->Deserialize(info["checker"]);
    }

    TStatusChecker::TFactory::TRegistrator<TSucceededChecker> RegistratorChecker("status_checker");
    TStatusChecker::TFactory::TRegistrator<TSucceededChecker> RegistratorSucceeded("succeeded_checker");
    TStatusChecker::TFactory::TRegistrator<TFailedChecker> RegistratorFailed("failed_checker");

    TFinishedChecker::TFactory::TRegistrator<TFinishedChecker> RegistratorFinished("finished_checker");

}
