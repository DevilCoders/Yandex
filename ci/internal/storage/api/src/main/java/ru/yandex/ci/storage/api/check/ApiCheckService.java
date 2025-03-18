package ru.yandex.ci.storage.api.check;

import java.time.Clock;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Set;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import lombok.Value;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.storage.api.cache.StorageApiCache;
import ru.yandex.ci.storage.core.CheckOuterClass;
import ru.yandex.ci.storage.core.Common.CheckStatus;
import ru.yandex.ci.storage.core.Common.StorageAttribute;
import ru.yandex.ci.storage.core.cache.StorageCoreCache;
import ru.yandex.ci.storage.core.check.CheckService;
import ru.yandex.ci.storage.core.check.RequirementsService;
import ru.yandex.ci.storage.core.check.tasks.CancelIterationFlowTask;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.constant.CheckStatusUtils;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.common.StorageRevision;
import ru.yandex.ci.storage.core.db.model.revision.RevisionEntity;
import ru.yandex.ci.storage.core.db.model.suite_restart.SuiteRestartEntity;
import ru.yandex.ci.storage.core.large.BazingaIterationId;
import ru.yandex.ci.storage.core.logbroker.event_producer.StorageEventsProducer;
import ru.yandex.ci.storage.core.sharding.ShardingSettings;
import ru.yandex.commune.bazinga.BazingaTaskManager;

public class ApiCheckService extends CheckService {
    private static final Logger log = LoggerFactory.getLogger(ApiCheckService.class);

    private final Clock clock;
    private final CiStorageDb db;
    private final ArcService arcService;
    private final String environment;
    private final StorageApiCache cache;
    private final StorageEventsProducer storageEventsProducer;
    private final int numberOfShardOutPartitions;
    private final BazingaTaskManager bazingaTaskManager;
    private final ShardingSettings shardingSettings;

    public ApiCheckService(
            Clock clock,
            RequirementsService requirementsService,
            @Nonnull CiStorageDb db,
            @Nonnull ArcService arcService,
            @Nonnull String environment,
            StorageApiCache cache,
            StorageEventsProducer storageEventsProducer,
            int numberOfShardOutPartitions,
            BazingaTaskManager bazingaTaskManager,
            ShardingSettings shardingSettings
    ) {
        super(requirementsService);
        this.clock = clock;
        this.db = db;
        this.arcService = arcService;
        this.environment = environment;
        this.cache = cache;
        this.storageEventsProducer = storageEventsProducer;
        this.numberOfShardOutPartitions = numberOfShardOutPartitions;
        this.bazingaTaskManager = bazingaTaskManager;
        this.shardingSettings = shardingSettings;
    }

    public CheckEntity register(CheckEntity check) {
        var leftCommit = arcService.getCommit(ArcRevision.parse(check.getLeft().getRevision()));
        var rightCommit = arcService.getCommit(ArcRevision.parse(check.getRight().getRevision()));
        log.info("left revision: {}", check.getLeft().getRevision());
        log.info("right revision: {}", check.getRight().getRevision());

        Preconditions.checkNotNull(leftCommit, "Commit not found: %s", check.getLeft().getRevision());
        Preconditions.checkNotNull(rightCommit, "Commit not found: %s", check.getRight().getRevision());

        if (leftCommit.getSvnRevision() == 0) {
            leftCommit = leftCommit.toBuilder().svnRevision(check.getLeft().getRevisionNumber()).build();
        }
        if (rightCommit.getSvnRevision() == 0) {
            rightCommit = rightCommit.toBuilder().svnRevision(check.getRight().getRevisionNumber()).build();
        }

        var revisions = new ArrayList<RevisionEntity>(2);
        var leftBranch = ArcBranch.ofString(check.getLeft().getBranch());
        var rightBranch = ArcBranch.ofString(check.getRight().getBranch());
        if (leftBranch.isRelease() || leftBranch.isTrunk()) {
            revisions.add(new RevisionEntity(leftBranch.getBranch(), leftCommit));
        }

        if (rightBranch.isRelease() || rightBranch.isTrunk()) {
            revisions.add(new RevisionEntity(rightBranch.getBranch(), rightCommit));
        }

        if (!revisions.isEmpty()) {
            this.db.currentOrTx(() -> db.revisions().bulkUpsert(revisions, revisions.size()));
        }

        final var left = leftCommit;
        final var right = rightCommit;
        return this.db.currentOrTx(() -> registerInTx(check, left, right));
    }

    public List<CheckEntity> findChecksByRevisions(String leftRevision, String rightRevision, Set<String> tags) {
        return this.db.currentOrReadOnly(() -> findChecksByRevisionsInTx(leftRevision, rightRevision, tags));
    }

    public CheckEntity onCancelRequested(CheckEntity.Id checkId) {
        return cache.modifyWithDbTxAndGet(cache -> onCancelRequestedInTx(cache, checkId, null).getCheck());
    }

    public CheckEntity onUserCancelRequested(CheckEntity.Id checkId, String cancelledBy) {
        return cache.modifyWithDbTxAndGet(cache -> {
            var cancelRespond = onCancelRequestedInTx(cache, checkId, cancelledBy);
            cancelRespond.getActiveIterations().stream()
                    .filter(x -> !x.getId().isMetaIteration())
                    .forEach(
                            iteration -> bazingaTaskManager.schedule(
                                    new CancelIterationFlowTask(new BazingaIterationId(iteration.getId()))
                            )
                    );

            return cancelRespond.getCheck();
        });
    }

    private CancelRespond onCancelRequestedInTx(
            StorageCoreCache.Modifiable cache, CheckEntity.Id checkId, @Nullable String cancelledBy
    ) {
        log.info("Cancelling check {}, by {}", checkId, cancelledBy == null ? "-" : cancelledBy);
        var check = cache.checks().getFreshOrThrow(checkId);
        if (CheckStatusUtils.isActive(check.getStatus())) {
            check = check.withStatus(CheckStatus.CANCELLING);
            if (cancelledBy != null) {
                check = check.setAttribute(StorageAttribute.SA_CANCELLED_BY, cancelledBy);
            }
            cache.checks().writeThrough(check);
        }

        var activeIterations = this.getActiveIterations(cache.iterations(), checkId);
        if (activeIterations.size() == 0) {
            log.info("Check {} have zero active iterations", checkId);
            check = check.withStatus(CheckStatus.CANCELLED);
            cache.checks().writeThrough(check);
            return new CancelRespond(check, List.of());
        }

        log.info("Cancelling {} active iterations for {}", activeIterations.size(), checkId);

        activeIterations.forEach(iteration -> this.onCancelRequestedInTx(cache, iteration.getId()));
        // CI-3168 We should move events report to tms task, to make api faster and consistent on tx failure.
        activeIterations.forEach(iteration -> storageEventsProducer.onCancelRequested(iteration.getId()));

        return new CancelRespond(check, activeIterations);
    }

    public CheckIterationEntity onCancelRequested(CheckIterationEntity.Id iterationId) {
        return this.cache.modifyWithDbTxAndGet(cache -> onCancelRequestedInTx(cache, iterationId));
    }

    private CheckIterationEntity onCancelRequestedInTx(
            StorageCoreCache.Modifiable cache,
            CheckIterationEntity.Id iterationId
    ) {
        var iteration = cache.iterations().getFreshOrThrow(iterationId);
        if (CheckStatusUtils.isActive(iteration.getStatus())) {
            iteration = iteration.withStatus(CheckStatus.CANCELLING);
            cache.iterations().writeThrough(iteration);
        }
        return iteration;
    }

    private CheckEntity registerInTx(CheckEntity check, ArcCommit leftCommit, ArcCommit rightCommit) {
        var existingChecks = this.db.checks().find(
                leftCommit.getRevision().getCommitId(),
                rightCommit.getRevision().getCommitId(),
                check.getTags()
        );

        if (!existingChecks.isEmpty()) {
            if (existingChecks.size() > 1) {
                log.error("Found more than one existing check, wanted to register {} ", check);
            }

            var existingCheck = existingChecks.get(0);
            log.info("Check already registered {}", existingCheck.getId());
            return existingCheck;
        }

        var id = check.getId().getId() == 0L ? this.db.checkIds().getId() : check.getId().getId();

        Preconditions.checkState(id != null, "No available id. Check id generation cron task.");

        var checkId = CheckEntity.Id.of(id);
        var shardOutPartition = checkId.distribute(numberOfShardOutPartitions);

        var newCheck = check.toBuilder()
                .id(checkId)
                .created(clock.instant())
                .shardOutPartition(shardOutPartition)
                .left(StorageRevision.from(check.getLeft().getBranch(), leftCommit))
                .right(StorageRevision.from(check.getRight().getBranch(), rightCommit))
                .shardingSettings(shardingSettings)
                .attributes(getAttributes(check))
                .runLargeTestsAfterDiscovery(check.getType() == CheckOuterClass.CheckType.TRUNK_POST_COMMIT)
                .build();

        log.info("Registering check: {}", newCheck);
        this.db.checks().save(newCheck);

        return newCheck;
    }

    private Map<StorageAttribute, String> getAttributes(CheckEntity check) {
        var attributes = new HashMap<>(check.getAttributes());
        attributes.put(StorageAttribute.SA_ENVIRONMENT, this.environment);
        attributes.put(StorageAttribute.SA_TESTENV_FINALIZATION_ALLOWED, Boolean.toString(false));
        return Map.copyOf(attributes);
    }

    private List<CheckEntity> findChecksByRevisionsInTx(String leftRevision, String rightRevision, Set<String> tags) {
        return this.db.checks().find(leftRevision, rightRevision, tags);
    }

    public List<CheckEntity> getChecks(Set<CheckEntity.Id> ids) {
        if (ids.isEmpty()) {
            return List.of();
        }

        return this.db.currentOrReadOnly(() -> this.db.checks().find(ids));
    }

    public List<CheckEntity> getCheckByTestenvIds(Collection<String> testenvIds) {
        if (testenvIds.isEmpty()) {
            return List.of();
        }

        return this.db.currentOrReadOnly(() -> {
            var ids = this.db.checkIterations()
                    .findByTestEnvIds(testenvIds).stream()
                    .map(x -> x.getId().getCheckId())
                    .collect(Collectors.toSet());

            if (ids.isEmpty()) {
                return List.of();
            }

            return this.db.checks().find(ids);
        });
    }

    public List<CheckIterationEntity> getIterationsByCheckIds(Set<CheckEntity.Id> ids) {
        if (ids.isEmpty()) {
            return List.of();
        }

        return this.db.currentOrReadOnly(() -> this.db.checkIterations().findByChecks(ids));
    }

    public List<SuiteRestartEntity> getSuiteRestarts(CheckIterationEntity.Id iterationId) {
        return this.db.currentOrReadOnly(() -> this.db.suiteRestarts().get(iterationId));
    }

    public List<CheckEntity> getPostCommitChecks(String revision) {
        return this.db.currentOrReadOnly(() -> this.db.checks().findPostCommits(revision));
    }

    public Optional<CheckEntity> findLastCheckByPullRequest(long pullRequestId) {
        return this.db.currentOrReadOnly(() -> this.db.checks().findLastCheckByPullRequest(pullRequestId));
    }

    @Value
    private static class CancelRespond {
        CheckEntity check;
        List<CheckIterationEntity> activeIterations;
    }
}
