package ru.yandex.ci.observer.api.statistics;

import java.time.Instant;
import java.util.ArrayList;
import java.util.List;
import java.util.Optional;
import java.util.Set;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.extern.slf4j.Slf4j;

import yandex.cloud.repository.kikimr.yql.YqlPredicate;
import yandex.cloud.repository.kikimr.yql.YqlStatementPart;
import ru.yandex.ci.client.arcanum.ArcanumClient;
import ru.yandex.ci.client.arcanum.ArcanumReviewDataDto;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.observer.api.statistics.model.IterationWithArcanumInfo;
import ru.yandex.ci.observer.core.db.CiObserverDb;
import ru.yandex.ci.observer.core.db.model.check_iterations.CheckIterationEntity;
import ru.yandex.ci.observer.core.db.model.check_tasks.CheckTaskEntity;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.CheckOuterClass;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.constant.CheckStatusUtils;
import ru.yandex.ci.storage.core.db.constant.CheckTypeUtils;

@Slf4j
public class DetailedStatisticsService {
    private static final Set<CheckOuterClass.CheckType> TYPES_WITH_REVIEW_ID = Set.of(
            CheckOuterClass.CheckType.TRUNK_PRE_COMMIT, CheckOuterClass.CheckType.BRANCH_PRE_COMMIT
    );
    private final CiObserverDb db;
    private final ArcanumClient arcanumClient;

    public DetailedStatisticsService(CiObserverDb db, ArcanumClient arcanumClient) {
        this.db = db;
        this.arcanumClient = arcanumClient;
    }

    public List<CheckIterationEntity> getDetailedAutocheckStagesByIterations(
            @Nonnull Instant from,
            @Nonnull Instant to,
            CheckOuterClass.CheckType checkType,
            List<CheckIteration.IterationType> iterationTypes,
            @Nullable String advisedPool,
            boolean hidePessimized,
            boolean hideFinished,
            boolean hideCancelled,
            List<String> authors
    ) {
        var addFilters = new ArrayList<YqlStatementPart<?>>();
        ifNotNull(advisedPool, () -> addFilters.add(YqlPredicate.where("advisedPool").eq(advisedPool)));
        addFilters.add(YqlPredicate.where("parentIterationNumber").isNull());

        if (hidePessimized) {
            addFilters.add(YqlPredicate.where("pessimized").eq(false));
        }
        if (hideFinished) {
            addFilters.add(YqlPredicate.or(
                    CheckStatusUtils.getRunning("status"),
                    YqlPredicate.where("hasUnfinishedRechecks").eq(true)
            ));
        }
        if (hideCancelled) {
            addFilters.add(YqlPredicate.where("status").neq(Common.CheckStatus.CANCELLED));
        }
        if (!iterationTypes.isEmpty()) {
            addFilters.add(concatPredicatesOr(iterationTypes, "id.iterType"));
        }

        addFilters.add(YqlPredicate.where("checkType").eq(checkType));

        if (!authors.isEmpty()) {
            addFilters.add(YqlPredicate.where("author").in(authors));
        }

        return db.readOnly(() -> {
            if (CheckTypeUtils.isPrecommitCheck(checkType)) {
                return db.iterations().findByCreated(from, to, addFilters);
            }
            return db.iterations().findByRightRevisionTimestamp(from, to, addFilters);
        });
    }

    public List<CheckTaskEntity> getIterationTasksWithRecheck(CheckIterationEntity.Id iterationId) {
        return db.readOnly(() -> {
            var iteration = db.iterations().get(iterationId);
            var recheckIterations = db.iterations().find(
                    iteration.getRecheckAggregationsByNumber().keySet().stream()
                            .map(n -> new CheckIterationEntity.Id(
                                    iterationId.getCheckId(), iterationId.getIterType(), n)
                            )
                            .collect(Collectors.toSet())
            );

            List<CheckTaskEntity> result = new ArrayList<>(getExpectedTasksByIteration(iteration));
            recheckIterations.forEach(i -> result.addAll(getExpectedTasksByIteration(i)));

            return result;
        });
    }

    public Optional<IterationWithArcanumInfo> getIteration(CheckIterationEntity.Id iterationId) {
        var iterationOpt = db.readOnly(() -> db.iterations().find(iterationId));
        if (iterationOpt.isEmpty()) {
            return Optional.empty();
        }
        var iteration = iterationOpt.get();
        var reviewId = TYPES_WITH_REVIEW_ID.contains(iteration.getCheckType())
                && iteration.getRight() != null
                ? ArcBranch.ofString(iteration.getRight().getBranch()).getPullRequestId()
                : null;

        Optional<ArcanumReviewDataDto> response = Optional.empty();
        try {
            if (reviewId != null) {
                response = arcanumClient.getReviewSummaryAndDescription(reviewId);
            }
        } catch (Exception ex) {
            log.error("Error during arcanum request with review id %d".formatted(reviewId), ex);
        }

        if (reviewId == null || response.isEmpty()) {
            return Optional.of(new IterationWithArcanumInfo(iteration, null, null));
        }

        return Optional.of(new IterationWithArcanumInfo(
                iteration,
                response.get().getSummary(),
                response.get().getDescription()
        ));
    }

    private List<CheckTaskEntity> getExpectedTasksByIteration(CheckIterationEntity iteration) {
        var expectedTasks = Set.copyOf(iteration.getExpectedCheckTasks());

        return db.tasks().getByIteration(iteration.getId()).stream()
                .filter(t -> expectedTasks.contains(
                        new CheckIterationEntity.CheckTaskKey(t.getJobName(), t.isRight())
                ))
                .collect(Collectors.toList());
    }

    private <T> void ifNotNull(@Nullable T object, Runnable runnable) {
        if (object != null) {
            runnable.run();
        }
    }

    private <T> YqlPredicate concatPredicatesOr(List<T> entities, String field) {
        return YqlPredicate.or(
                entities.stream()
                        .map(t -> YqlPredicate.where(field).eq(t))
                        .collect(Collectors.toList())
        );
    }
}
