package ru.yandex.ci.storage.core.check.tasks;

import java.time.Duration;

import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.api.storage.StorageApi;
import ru.yandex.ci.client.ci.CiClient;
import ru.yandex.ci.common.bazinga.AbstractOnetimeTask;
import ru.yandex.ci.core.proto.CiCoreProtoMappers;
import ru.yandex.ci.storage.core.cache.StorageCoreCache;
import ru.yandex.ci.storage.core.large.BazingaIterationId;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;


@Slf4j
public class CancelIterationFlowTask extends AbstractOnetimeTask<BazingaIterationId> {
    private StorageCoreCache<?> storageCache;
    private CiClient ciClient;

    private String environment;

    public CancelIterationFlowTask(StorageCoreCache<?> storageCache, CiClient ciClient, String environment) {
        super(BazingaIterationId.class);
        this.storageCache = storageCache;
        this.ciClient = ciClient;
        this.environment = environment;
    }

    public CancelIterationFlowTask(BazingaIterationId checkId) {
        super(checkId);
    }

    @Override
    protected void execute(BazingaIterationId bazingaIterationId, ExecutionContext context) {
        var iterationId = bazingaIterationId.getIterationId();

        var check = this.storageCache.checks().getFreshOrThrow(iterationId.getCheckId());
        if (!check.isFromEnvironment(environment)) {
            log.info(
                    "Skipped check {} because it is from different environment: {}",
                    check.getId(), check.getEnvironment()
            );
            return;
        }

        log.info("Cancelling flow for iteration {}", iterationId);
        var iteration = this.storageCache.iterations().getFreshOrThrow(iterationId);
        log.info("Iteration status: {}", iteration.getStatus());

        var actions = iteration.getInfo().getCiActionReferences();

        actions.forEach(
                action -> {
                    log.info("Cancelling flow: {}/{}", action.getFlowId(), action.getLaunchNumber());
                    ciClient.cancelFlow(
                            StorageApi.CancelFlowRequest.newBuilder()
                                    .setFlowProcessId(CiCoreProtoMappers.toProtoFlowProcessId(action.getFlowId()))
                                    .setNumber(action.getLaunchNumber())
                                    .build()
                    );
                }
        );
    }

    @Override
    public Duration getTimeout() {
        return Duration.ofMinutes(5);
    }

}
