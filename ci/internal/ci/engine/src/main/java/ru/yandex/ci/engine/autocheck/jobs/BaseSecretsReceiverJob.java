package ru.yandex.ci.engine.autocheck.jobs;

import ru.yandex.ci.core.launch.LaunchRuntimeInfo;
import ru.yandex.ci.core.security.YavToken;
import ru.yandex.ci.engine.flow.SecurityAccessService;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;

public abstract class BaseSecretsReceiverJob implements JobExecutor {

    protected abstract SecurityAccessService getSecurityAccessService();

    protected String fetchCiMainToken(LaunchRuntimeInfo runtimeInfo) {
        YavToken.Id tokenUuid = runtimeInfo.getYavTokenUidOrThrow();
        var service = getSecurityAccessService();
        return service.getYavSecret(tokenUuid).getCiToken();
    }
}
