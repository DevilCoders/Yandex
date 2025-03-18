package ru.yandex.ci.storage.core.db.model.check_iteration;

import java.time.Instant;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Comparator;
import java.util.List;
import java.util.Optional;
import java.util.Set;
import java.util.stream.Collectors;

import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;
import yandex.cloud.repository.kikimr.yql.YqlLimit;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;
import yandex.cloud.repository.kikimr.yql.YqlView;

import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.common.ydb.YqlPredicateCi;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.constant.CheckStatusUtils;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;

public class CheckIterationTable extends KikimrTableCi<CheckIterationEntity> {
    public CheckIterationTable(QueryExecutor executor) {
        super(CheckIterationEntity.class, executor);
    }


    public Optional<CheckIterationEntity> findLast(CheckEntity.Id id, CheckIteration.IterationType iterationType) {
        return this.find(
                YqlPredicate.where("id.checkId").eq(id).and("id.iterationType").eq(iterationType.getNumber())
        ).stream().max(Comparator.comparingInt(a -> a.getId().getNumber()));
    }

    public long countActive() {
        return this.count(
                CheckStatusUtils.getIsActive("status"),
                byStatusAndCreated()
        );
    }

    public long countActive(CheckEntity.Id id) {
        return this.count(
                YqlPredicate.and(
                        YqlPredicate.where("id.checkId").eq(id),
                        CheckStatusUtils.getIsActive("status")
                )
        );
    }

    public List<CheckIterationEntity> findActive(CheckEntity.Id id) {
        return this.find(
                YqlPredicate.and(
                        YqlPredicate.where("id.checkId").eq(id),
                        CheckStatusUtils.getIsActive("status")
                )
        );
    }

    public List<CheckIterationEntity> findRunningStartedBefore(Instant beforeTime, int limit) {
        // todo replace with CheckStatusUtils.getRunning("status"), when ydb supports index scan with IN(in middle).

        var created = this.find(
                YqlPredicate
                        .where("status").eq(Common.CheckStatus.CREATED)
                        .and("created").lte(beforeTime),
                YqlLimit.range(0, limit),
                byStatusAndCreated()
        );

        var running = this.find(
                YqlPredicate
                        .where("status").eq(Common.CheckStatus.RUNNING)
                        .and("created").lte(beforeTime),
                YqlLimit.range(0, limit),
                byStatusAndCreated()
        );

        var result = new ArrayList<CheckIterationEntity>();
        result.addAll(created);
        result.addAll(running);

        return result;
    }

    public List<CheckIterationEntity> findByCheck(CheckEntity.Id id) {
        return this.find(YqlPredicate.where("id.checkId").eq(id));
    }

    public List<CheckIterationEntity> findByChecks(Set<CheckEntity.Id> ids) {
        if (ids.size() == 1) {
            return this.find(YqlPredicate.where("id.checkId").eq(ids.iterator().next()));
        }

        return this.find(
                YqlPredicateCi.in("id.checkId", ids.stream().map(CheckEntity.Id::getId).collect(Collectors.toSet()))
        );
    }

    private YqlView byStatusAndCreated() {
        return YqlView.index(CheckIterationEntity.IDX_BY_STATUS_AND_CREATED);
    }

    public List<CheckIterationEntity> findByTestEnvIds(Collection<String> ids) {
        if (ids.isEmpty()) {
            return List.of();
        }

        if (ids.size() == 1) {
            return this.find(
                    YqlPredicate.where("testenvId").eq(ids.iterator().next()),
                    YqlView.index(CheckIterationEntity.IDX_BY_TEST_ENV_ID)
            );
        }

        return this.find(YqlPredicateCi.in("testenvId", ids), YqlView.index(CheckIterationEntity.IDX_BY_TEST_ENV_ID));
    }
}
