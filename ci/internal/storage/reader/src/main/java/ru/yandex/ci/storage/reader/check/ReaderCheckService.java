package ru.yandex.ci.storage.reader.check;

import java.time.LocalTime;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.function.Function;
import java.util.stream.Collectors;

import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.core.pr.ArcanumCheckType;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.Common.CheckStatus;
import ru.yandex.ci.storage.core.check.ArcanumCheckStatus;
import ru.yandex.ci.storage.core.check.CheckService;
import ru.yandex.ci.storage.core.check.RequirementsService;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.constant.CheckStatusUtils;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.metrics.Metrics;
import ru.yandex.ci.storage.core.db.model.check_merge_requirements.MergeInterval;
import ru.yandex.ci.storage.core.db.model.check_task.CheckTaskEntity;
import ru.yandex.ci.storage.core.db.model.check_task_statistics.CheckTaskStatisticsEntity;
import ru.yandex.ci.storage.core.db.model.chunk_aggregate.ChunkAggregateEntity;
import ru.yandex.ci.storage.core.db.model.skipped_check.SkippedCheckEntity;
import ru.yandex.ci.storage.core.logbroker.badge_events.BadgeEventsProducer;
import ru.yandex.ci.storage.reader.cache.ReaderCache;
import ru.yandex.ci.storage.reader.message.main.ReaderStatistics;
import ru.yandex.ci.storage.reader.other.MetricAggregationService;

@Slf4j
public class ReaderCheckService extends CheckService {
    private static final int BULK_LIMIT = 8192;

    // We can not fully rely on cache as api and other readers can make modifications to db.
    // We can rely on cache for task statistics, as it divided by hosts in db.
    private final ReaderCache cache;
    private final ReaderStatistics readerStatistics;
    private final CiStorageDb db;
    private final BadgeEventsProducer badgeEventsProducer;

    private final MetricAggregationService metricAggregationService;

    public ReaderCheckService(
            RequirementsService requirementsService,
            ReaderCache cache,
            ReaderStatistics readerStatistics,
            CiStorageDb db,
            BadgeEventsProducer badgeEventsProducer,
            MetricAggregationService metricAggregationService
    ) {
        super(requirementsService);
        this.cache = cache;
        this.readerStatistics = readerStatistics;
        this.db = db;
        this.badgeEventsProducer = badgeEventsProducer;
        this.metricAggregationService = metricAggregationService;
    }

    public void processMetrics(
            CheckTaskEntity.Id taskId, int partition, Metrics metrics
    ) {
        cache.modifyWithDbTx(cache -> addMetricsInTx(cache, taskId, partition, metrics));
    }

    private void addMetricsInTx(
            ReaderCache.Modifiable cache,
            CheckTaskEntity.Id taskId,
            int partition,
            Metrics metrics
    ) {
        final var task = cache.checkTasks().getFreshOrThrow(taskId);
        final var iteration = cache.iterations().getFreshOrThrow(taskId.getIterationId());

        var taskMetrics = new HashMap<>(task.getPartitionsMetrics());
        var previousTaskMetrics = taskMetrics.getOrDefault(partition, Metrics.EMPTY);
        var updatedTaskMetrics = metricAggregationService.merge(previousTaskMetrics, metrics);
        taskMetrics.put(partition, updatedTaskMetrics);
        cache.checkTasks().writeThrough(task.withPartitionsMetrics(taskMetrics));

        cache.iterations().writeThrough(
                iteration.withStatistics(iteration.getStatistics().withMetrics(
                                metricAggregationService.aggregate(
                                        iteration.getStatistics().getMetrics(), previousTaskMetrics, updatedTaskMetrics
                                )
                        )
                )
        );

        var metaIteration = cache.iterations().getFresh(taskId.getIterationId().toMetaId());
        if (metaIteration.isEmpty()) {
            return;
        }

        cache.iterations().writeThrough(
                metaIteration.get().withStatistics(
                        metaIteration.get().getStatistics().withMetrics(
                                metricAggregationService.aggregate(
                                        metaIteration.get().getStatistics().getMetrics(),
                                        previousTaskMetrics,
                                        updatedTaskMetrics
                                )
                        )
                )
        );
    }

    public void updateTaskStatistics(List<CheckTaskStatisticsEntity> result) {
        this.cache.modifyWithDbTx(cache -> {
            this.db.checkTaskStatistics().bulkUpsertWithRetries(
                    result, BULK_LIMIT, (e) -> this.readerStatistics.onBulkInsertError()
            );
            cache.taskStatistics().put(result);
        });
    }

    public void processPessimize(CheckIterationEntity.Id iterationId, String info) {
        cache.modifyWithDbTx(cache -> {
            processPessimizeInTx(cache, iterationId, info);
            if (iterationId.getNumber() > 1) {
                processPessimizeInTx(cache, iterationId.toMetaId(), info);
            }
        });
    }

    private void processPessimizeInTx(
            ReaderCache.Modifiable cache, CheckIterationEntity.Id iterationId, String info
    ) {
        final var iteration = cache.iterations().getFreshOrThrow(iterationId);

        if (CheckStatusUtils.isCompleted(iteration.getStatus())) {
            log.warn("Iteration {} is not running, skipping pessimize", iterationId);
            return;
        }

        var iterationInfo = iteration.getInfo();

        if (!iterationInfo.isPessimized()) {
            cache.iterations().writeThrough(iteration.withInfo(iterationInfo.pessimize(info)));
            this.requirementsService.scheduleRequirement(
                    cache,
                    iterationId.getCheckId(),
                    ArcanumCheckType.AUTOCHECK_PESSIMIZED,
                    ArcanumCheckStatus.success(ArcanumCheckType.AUTOCHECK_PESSIMIZED.getDescription()),
                    List.of(new MergeInterval(LocalTime.of(16, 0), LocalTime.of(9, 0)))
            );
        }
    }

    public void processDistbuildStartedInTx(ReaderCache.Modifiable cache, CheckTaskEntity.Id taskId) {
        log.info("Distbuild started for {}", taskId);

        var task = cache.checkTasks().getFreshOrThrow(taskId);
        final var iteration = cache.iterations().getFreshOrThrow(taskId.getIterationId());
        var metaIteration = cache.iterations().getFresh(taskId.getIterationId());

        if (task.getStatus().equals(CheckStatus.CREATED)) {
            cache.checkTasks().writeThrough(task.withStatus(CheckStatus.RUNNING));

            if (iteration.getStatus() == CheckStatus.CREATED) {
                cache.iterations().writeThrough(iteration.run());
                var check = cache.checks().getFreshOrThrow(taskId.getIterationId().getCheckId());
                if (check.getStatus() == CheckStatus.CREATED) {
                    cache.checks().writeThrough(check.run());
                }

                metaIteration.ifPresent(i -> cache.iterations().writeThrough(i.run()));
            }
        }
    }

    public Collection<ChunkAggregateEntity> getMetaIterationAggregates(
            ReaderCache.Modifiable cache,
            CheckIterationEntity.Id iterationId
    ) {
        var aggregates = cache.chunkAggregatesGroupedByIteration()
                .getByIterationId(iterationId.toIterationId(1)).stream()
                .collect(Collectors.toMap(x -> x.getId().getChunkId(), Function.identity()));

        aggregates.putAll(
                cache.chunkAggregatesGroupedByIteration().getByIterationId(iterationId).stream()
                        .collect(Collectors.toMap(x -> x.getId().getChunkId(), Function.identity()))
        );

        return aggregates.values();
    }

    public void skipCheck(CheckEntity.Id checkId, String reason) {
        this.cache.modifyWithDbTx(
                cache -> cache.skippedChecks().writeThrough(
                        new SkippedCheckEntity(SkippedCheckEntity.Id.of(checkId), reason)
                )
        );
    }

    public void sendCheckFailedInAdvance(ReaderCache.Modifiable cache, CheckIterationEntity iteration) {
        var check = cache.checks().getOrThrow(iteration.getId().getCheckId());
        if (check.isFirstFailSent()) {
            return;
        }

        if (!iteration.getStatistics().hasUnrecoverableErrors()) {
            return;
        }

        check = cache.checks().getFreshOrThrow(iteration.getId().getCheckId());
        if (check.isFirstFailSent()) {
            return;
        }

        check = check.setAttribute(Common.StorageAttribute.SA_FIRST_FAIL_SENT, Boolean.TRUE.toString());

        badgeEventsProducer.onCheckFirstFail(check, iteration);

        cache.checks().writeThrough(check);
    }
}
