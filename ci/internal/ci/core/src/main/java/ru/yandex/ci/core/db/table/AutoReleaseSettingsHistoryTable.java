package ru.yandex.ci.core.db.table;

import java.time.Instant;
import java.util.Collection;
import java.util.List;
import java.util.Map;
import java.util.function.Function;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import lombok.Value;
import one.util.streamex.StreamEx;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.repository.db.Table.View;
import yandex.cloud.repository.kikimr.DbType;
import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;
import yandex.cloud.repository.kikimr.yql.YqlLimit;
import yandex.cloud.repository.kikimr.yql.YqlOrderBy;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;
import yandex.cloud.repository.kikimr.yql.YqlStatementPart;

import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.db.model.AutoReleaseSettingsHistory;

public class AutoReleaseSettingsHistoryTable extends KikimrTableCi<AutoReleaseSettingsHistory> {

    public AutoReleaseSettingsHistoryTable(QueryExecutor executor) {
        super(AutoReleaseSettingsHistory.class, executor);
    }

    @Nullable
    public AutoReleaseSettingsHistory findLatest(CiProcessId processId) {
        return findTopOrderedByTimestampDesc(processId)
                .stream()
                .findFirst()
                .orElse(null);
    }

    public Map<CiProcessId, AutoReleaseSettingsHistory> findLatest(Collection<CiProcessId> processIds) {
        List<String> processIdStrings = StreamEx.of(processIds)
                .map(CiProcessId::asString)
                .toList();

        List<YqlStatementPart<?>> filter = List.of(YqlPredicate.where("id.processId").in(processIdStrings));

        var ids = groupBy(
                LatestId.class, List.of("processId", "max(`timestamp`) as latestTs"), List.of("processId"), filter)
                .stream()
                .map(LatestId::toId)
                .collect(Collectors.toSet());

        return StreamEx.of(find(ids))
                .toMap(h -> CiProcessId.ofString(h.getId().getProcessId()), Function.identity());
    }

    public List<AutoReleaseSettingsHistory> findTopOrderedByTimestampDesc(CiProcessId processId) {
        return find(
                YqlPredicate.where("id.processId").eq(processId.asString()),
                YqlOrderBy.orderBy("id.timestamp", YqlOrderBy.SortOrder.DESC),
                YqlLimit.top(1));
    }

    @Value
    static class LatestId implements View {
        @Column(name = "processId", dbType = DbType.UTF8)
        String processId;

        @Column(name = "latestTs", dbType = DbType.TIMESTAMP)
        Instant timestamp;

        @Column(name = "count", dbType = "UINT64")
        long count;

        public AutoReleaseSettingsHistory.Id toId() {
            return AutoReleaseSettingsHistory.Id.of(processId, timestamp);
        }
    }
}
