package ru.yandex.ci.storage.core.large;

import java.time.Duration;

import javax.annotation.Nonnull;

import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.api.proto.Common;
import ru.yandex.ci.api.storage.StorageApi;
import ru.yandex.ci.client.ci.CiClient;
import ru.yandex.ci.common.bazinga.AbstractOnetimeTask;
import ru.yandex.ci.storage.core.cache.StorageCoreCache;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;

@Slf4j
public class MarkDiscoveredCommitTask extends AbstractOnetimeTask<BazingaCheckId> {
    private CiClient ciClient;
    private StorageCoreCache<?> storageCache;

    public MarkDiscoveredCommitTask(CiClient ciClient, StorageCoreCache<?> storageCache) {
        super(BazingaCheckId.class);
        this.ciClient = ciClient;
        this.storageCache = storageCache;
    }

    public MarkDiscoveredCommitTask(@Nonnull CheckEntity.Id checkId) {
        super(new BazingaCheckId(checkId));
    }

    @Override
    protected void execute(BazingaCheckId params, ExecutionContext context) {
        var check = storageCache.checks().getOrThrow(params.getCheckId());
        var revision = check.getRight().getRevision();
        var commit = Common.CommitId.newBuilder()
                .setCommitId(revision)
                .build();

        var request = StorageApi.MarkDiscoveredCommitRequest.newBuilder()
                .setRevision(commit)
                .build();

        log.info("Mark commit as discovered: {}", revision);
        ciClient.markDiscoveredCommit(request);
    }

    @Override
    public Duration getTimeout() {
        return Duration.ofMinutes(5);
    }

}
