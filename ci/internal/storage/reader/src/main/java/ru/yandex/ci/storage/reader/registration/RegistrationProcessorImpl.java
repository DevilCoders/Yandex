package ru.yandex.ci.storage.reader.registration;

import java.time.Duration;
import java.time.Instant;
import java.util.ArrayList;

import lombok.AllArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.storage.core.EventsStreamMessages;
import ru.yandex.ci.storage.core.check.CreateIterationParams;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.check_task.CheckTaskEntity;
import ru.yandex.ci.storage.core.db.model.revision.RevisionEntity;
import ru.yandex.ci.storage.core.exceptions.CheckIsReadonlyException;
import ru.yandex.ci.storage.core.exceptions.IterationInWrongStatusException;
import ru.yandex.ci.storage.core.exceptions.TaskAlreadyRegisteredException;
import ru.yandex.ci.storage.core.proto.CheckProtoMappers;
import ru.yandex.ci.storage.core.registration.RegistrationProcessor;
import ru.yandex.ci.storage.core.sharding.ShardingSettings;
import ru.yandex.ci.storage.reader.cache.ReaderCache;
import ru.yandex.ci.storage.reader.check.ReaderCheckService;

@Slf4j
@AllArgsConstructor
public class RegistrationProcessorImpl implements RegistrationProcessor {
    private final CiStorageDb db;
    private final ReaderCache storageCache;
    private final ReaderCheckService checkService;
    private final int numberOfShardOutPartitions;
    private final ShardingSettings shardingSettings;

    @Override
    public void processMessage(EventsStreamMessages.RegistrationMessage message, Instant timestamp) {
        var lag = Duration.between(Instant.now(), timestamp).getSeconds();
        log.info(
                "Receive registration message for check {}, lag {}", message.getCheck().getId(), lag
        );

        var check = CheckProtoMappers.toCheck(message.getCheck());
        if (check.getId().sampleOut(shardingSettings)) {
            log.info("Check {} was removed by sampling", check.getId());
            return;
        }

        var shardOutPartition = check.getId().distribute(numberOfShardOutPartitions);
        check = check.toBuilder()
                .shardOutPartition(shardOutPartition)
                .shardingSettings(shardingSettings)
                .build();

        var iteration = CheckProtoMappers.toCheckIteration(message.getIteration());

        try {
            register(message, check, iteration);
        } catch (CheckIsReadonlyException exception) {
            log.info("Can't register iteration {}: {}", iteration.getId(), exception.getMessage());
        }
    }

    private void register(
            EventsStreamMessages.RegistrationMessage message, CheckEntity check, CheckIterationEntity iteration
    ) {
        if (message.hasTask()) {
            var checkTask = CheckProtoMappers.toCheckTask(message.getTask());

            log.info(
                    "Registering check {}, iteration {}, task {}",
                    check.getId(), iteration.getId(), checkTask.getId()
            );

            register(check, iteration, checkTask);
        } else {
            log.info("Registering check {}, iteration {}", check.getId(), iteration.getId());
            register(check, iteration);
        }
    }

    private void register(
            CheckEntity check,
            CheckIterationEntity iteration,
            CheckTaskEntity checkTask
    ) {
        register(check, iteration);

        storageCache.modifyWithDbTx(cache -> {
            try {
                checkService.registerTaskInTx(cache, checkTask);
            } catch (TaskAlreadyRegisteredException ignored) {
                log.info("Task {} already registered", checkTask.getId());
            } catch (IterationInWrongStatusException exception) {
                log.info("Can't register task {}: {}", checkTask.getId(), exception.getMessage());
            }
        });
    }

    private void register(CheckEntity check, CheckIterationEntity iteration) {
        var iterationId = iteration.getId();

        storageCache.modifyWithDbTx(
                cache -> {
                    if (
                            cache.checks().get(check.getId()).isEmpty() && // first check without db call
                                    cache.checks().getFresh(check.getId()).isEmpty()
                    ) {
                        cache.checks().writeThrough(check);
                        log.info("Check {} registered", check.getId());

                        // todo https://st.yandex-team.ru/CI-3494
                        var revisions = new ArrayList<RevisionEntity>(2);
                        var leftBranch = ArcBranch.ofString(check.getLeft().getBranch());
                        var rightBranch = ArcBranch.ofString(check.getRight().getBranch());
                        if (leftBranch.isRelease() || leftBranch.isTrunk()) {
                            revisions.add(
                                    new RevisionEntity(
                                            leftBranch.getBranch(),
                                            ArcCommit.builder()
                                                    .id(ArcCommit.Id.of(check.getLeft().getRevision()))
                                                    .message("")
                                                    .author("")
                                                    .svnRevision(check.getLeft().getRevisionNumber())
                                                    .createTime(Instant.now())
                                                    .build()
                                    )
                            );
                        }

                        if (rightBranch.isRelease() || rightBranch.isTrunk()) {
                            revisions.add(
                                    new RevisionEntity(
                                            leftBranch.getBranch(),
                                            ArcCommit.builder()
                                                    .id(ArcCommit.Id.of(check.getRight().getRevision()))
                                                    .message("")
                                                    .author("")
                                                    .svnRevision(check.getRight().getRevisionNumber())
                                                    .createTime(Instant.now())
                                                    .build()
                                    )
                            );
                        }

                        this.db.currentOrTx(() -> db.revisions().bulkUpsert(revisions, revisions.size()));
                    } else {
                        log.info("Check {} already registered", check.getId());
                    }
                }
        );

        storageCache.modifyWithDbTx(
                cache -> {
                    if (
                            cache.iterations().get(iterationId).isPresent() || // first check without db call
                                    cache.iterations().getFresh(iterationId).isPresent()
                    ) {
                        log.info("Iteration {} already registered", iterationId);
                        return;
                    }

                    checkService.registerIterationInTx(
                            cache,
                            iterationId,
                            CreateIterationParams.builder()
                                    .info(iteration.getInfo())
                                    .startedBy(iteration.getStartedBy())
                                    .expectedTasks(iteration.getExpectedTasks())
                                    .build()
                    );
                    log.info("Check iteration {} registered", iterationId);
                }
        );
    }
}
