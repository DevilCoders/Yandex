package ru.yandex.ci.storage.core.db.model.check_text_search;

import java.util.List;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import yandex.cloud.repository.db.readtable.ReadTableParams;
import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;
import yandex.cloud.repository.kikimr.yql.YqlLimit;
import yandex.cloud.repository.kikimr.yql.YqlOrderBy;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;
import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;

public class CheckTextSearchTable extends KikimrTableCi<CheckTextSearchEntity> {
    public CheckTextSearchTable(QueryExecutor executor) {
        super(CheckTextSearchEntity.class, executor);
    }

    public List<String> findSuggest(
            CheckIterationEntity.Id iterationId, Common.CheckSearchEntityType entityType, String value
    ) {
        return this.findDistinct(
                        CheckTextSearchEntity.DistinctView.class,
                        List.of(
                                YqlPredicate
                                        .where("id.checkId").eq(iterationId.getCheckId().getId())
                                        .and("id.iterationType").eq(iterationId.getIterationType().getNumber())
                                        .and("id.entityType").eq(entityType.getNumber())
                                        .and("id.value").like(value.toLowerCase().replace("*", "%")),
                                YqlOrderBy.orderBy("id.checkId", "id.iterationType", "id.entityType", "id.value"),
                                YqlLimit.range(0, 128)
                        )
                ).stream()
                .map(x -> x.getDisplayValue().isEmpty() ? x.getId().getValue() : x.getDisplayValue())
                .collect(Collectors.toList());
    }

    public Stream<CheckTextSearchEntity.Id> fetchIdsByCheck(CheckEntity.Id checkId, int limit) {
        var fromId = CheckTextSearchEntity.Id.builder()
                .checkId(checkId.getId()).iterationType(Integer.MIN_VALUE).build();
        var toId = CheckTextSearchEntity.Id.builder()
                .checkId(checkId.getId()).iterationType(Integer.MAX_VALUE).build();

        return this.readTableIds(
                ReadTableParams.<CheckTextSearchEntity.Id>builder()
                        .fromKey(fromId)
                        .toKey(toId)
                        .toInclusive(true)
                        .fromInclusive(true)
                        .ordered()
                        .rowLimit(limit)
                        .build()
        );
    }
}
