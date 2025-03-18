package ru.yandex.ci.storage.core.db.model.test_mute;

import java.time.Instant;
import java.util.ArrayList;
import java.util.List;
import java.util.Optional;

import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;
import yandex.cloud.repository.kikimr.yql.YqlLimit;
import yandex.cloud.repository.kikimr.yql.YqlOrderBy;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;
import yandex.cloud.repository.kikimr.yql.YqlView;

import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.common.ydb.YdbUtils;
import ru.yandex.ci.storage.core.db.model.test_status.TestStatusEntity;

public class TestMuteTable extends KikimrTableCi<TestMuteEntity> {

    public TestMuteTable(QueryExecutor executor) {
        super(TestMuteEntity.class, executor);
    }

    public List<TestMuteEntity> getLastActions(Instant since, int limit) {
        var mutes = new ArrayList<TestMuteEntity>(YdbUtils.RESULT_ROW_LIMIT);
        while (mutes.size() < limit) {
            var loaded = this.getLastActionsFitsOneRequest(since);
            mutes.addAll(loaded);
            if (loaded.size() < YdbUtils.RESULT_ROW_LIMIT) {
                break;
            }
            since = loaded.get(loaded.size() - 1).getId().getTimestamp();
        }

        return mutes;
    }

    private List<TestMuteEntity> getLastActionsFitsOneRequest(Instant since) {
        return this.find(
                YqlPredicate.where("id.timestamp").gt(since),
                YqlView.index(TestMuteEntity.IDX_BY_TIMESTAMP),
                YqlOrderBy.orderBy("id.timestamp", YqlOrderBy.SortOrder.ASC),
                YqlLimit.top(YdbUtils.RESULT_ROW_LIMIT)
        );
    }

    public Optional<TestMuteEntity> getLastAction(TestStatusEntity.Id id) {
        return this.find(
                YqlPredicate.where("id.testId").eq(id),
                YqlOrderBy.orderBy("id", YqlOrderBy.SortOrder.DESC),
                YqlLimit.top(1)
        ).stream().findFirst();
    }
}
