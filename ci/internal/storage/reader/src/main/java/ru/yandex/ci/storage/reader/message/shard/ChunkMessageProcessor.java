package ru.yandex.ci.storage.reader.message.shard;

import java.util.HashSet;
import java.util.List;
import java.util.Optional;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;

import com.ibm.icu.impl.Pair;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import yandex.cloud.repository.db.exception.UnavailableException;

import ru.yandex.ci.storage.core.ShardOut;
import ru.yandex.ci.storage.core.db.constant.CheckStatusUtils;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.chunk_aggregate.ChunkAggregateEntity;
import ru.yandex.ci.storage.core.proto.CheckProtoMappers;
import ru.yandex.ci.storage.reader.cache.ReaderCache;
import ru.yandex.ci.storage.reader.check.CheckFinalizationService;
import ru.yandex.ci.storage.reader.check.ReaderCheckService;
import ru.yandex.ci.storage.reader.message.main.ReaderStatistics;
import ru.yandex.ci.util.HostnameUtils;
import ru.yandex.ci.util.Retryable;

@Slf4j
@RequiredArgsConstructor
public class ChunkMessageProcessor {
    @Nonnull
    private final ReaderCheckService checkService;

    @Nonnull
    private final CheckFinalizationService checkFinalizationService;
    @Nonnull
    private final ReaderCache readerCache;
    @Nonnull
    private final ReaderStatistics statistics;

    public void processChunkFinishedMessages(List<ShardOut.ShardOutMessage> messages) {
        if (messages.isEmpty()) {
            return;
        }

        var byIterationId = messages.stream()
                .map(ShardOut.ShardOutMessage::getFinished)
                .map(m -> Pair.of(
                                CheckProtoMappers.toAggregate(m.getAggregate()),
                                m.getMetaAggregate().getId().getIterationId().getCheckId().isEmpty() ?
                                        Optional.<ChunkAggregateEntity>empty() :
                                        Optional.of(CheckProtoMappers.toAggregate(m.getMetaAggregate()))

                        )
                )
                .collect(Collectors.groupingBy(x -> x.first.getId().getIterationId()));

        log.info("Processing {} finish messages for {} iterations", messages.size(), byIterationId.size());

        for (var group : byIterationId.entrySet()) {
            checkFinalizationService.processChunkFinished(group.getKey(), group.getValue());
        }
    }

    public void processAggregateMessages(List<ShardOut.ShardOutMessage> messages) {
        if (messages.isEmpty()) {
            return;
        }

        var aggregatesByIterations = messages.stream().map(m -> CheckProtoMappers.toAggregate(m.getAggregate()))
                .collect(Collectors.groupingBy(x -> x.getId().getIterationId()));

        log.info(
                "Processing shard aggregates, number of aggregates: {}, number of iterations: {}",
                messages.size(), aggregatesByIterations.keySet().size()
        );

        for (var entry : aggregatesByIterations.entrySet()) {
            Retryable.retryUntilInterruptedOrSucceeded(
                    () -> readerCache.modifyWithDbTx(
                            cache -> processAggregates(cache, entry.getKey(), entry.getValue())
                    ),
                    (e) -> {
                        if (e instanceof UnavailableException) {
                            log.warn("Db unavailable {}", e.getMessage());
                            this.statistics.onDbUnavailableError();
                        } else {
                            log.error("Failed", e);
                            this.statistics.getShard().onReadFailed();
                        }
                    },
                    true
            );

        }
    }

    private void processAggregates(
            ReaderCache.Modifiable cache,
            CheckIterationEntity.Id iterationId,
            List<ChunkAggregateEntity> aggregates
    ) {
        var iteration = cache.iterations().getFreshOrThrow(iterationId);

        if (!CheckStatusUtils.isRunning(iteration.getStatus())) {
            if (CheckStatusUtils.isCompleted(iteration.getStatus())) {
                log.info(
                        "Skipping aggregate because iteration {} is not active, status: {}",
                        iteration.getId(), iteration.getStatus()
                );
                cache.iterations().put(iteration); // refresh cache, to stop receiving data for completed iteration
            } else {
                log.info(
                        "Skipping aggregate because iteration {} is finishing, status: {}",
                        iteration.getId(), iteration.getStatus()
                );
            }

            return;
        }

        for (var aggregate : aggregates) {
            var chunkType = aggregate.getId().getChunkId().getChunkType();
            var testTypeStatistics = iteration.getTestTypeStatistics().get(chunkType);
            if (testTypeStatistics.isCompleted()) {
                log.info(
                        "Skipping aggregate for {} because chunk type {} completed on {}",
                        iteration.getId(), chunkType, testTypeStatistics.getCompleted()
                );
            } else {
                cache.chunkAggregatesGroupedByIteration().put(aggregate);
            }
        }

        if (!iteration.getShardOutProcessedBy().contains(HostnameUtils.getShortHostname())) {
            var readers = new HashSet<>(iteration.getShardOutProcessedBy());
            readers.add(HostnameUtils.getShortHostname());
            iteration = iteration.withShardOutProcessedBy(readers);
        }

        var iterationAggregates = iterationId.isMetaIteration() ?
                checkService.getMetaIterationAggregates(cache, iterationId) :
                cache.chunkAggregatesGroupedByIteration().getByIterationId(iterationId);

        var updatedIteration = CheckIterationEntity.sumOf(iteration, iterationAggregates);
        checkService.sendCheckFailedInAdvance(cache, updatedIteration);
        cache.iterations().writeThrough(updatedIteration);
    }


}
