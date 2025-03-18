package ru.yandex.ci.storage.core.db.model.suite_restart;

import java.util.List;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import com.google.common.primitives.UnsignedLong;

import yandex.cloud.repository.db.readtable.ReadTableParams;
import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;
import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;

public class SuiteRestartTable extends KikimrTableCi<SuiteRestartEntity> {

    public SuiteRestartTable(QueryExecutor executor) {
        super(SuiteRestartEntity.class, executor);
    }

    public List<SuiteRestartEntity> get(CheckIterationEntity.Id iterationId) {
        return this.readTable(
                ReadTableParams.<SuiteRestartEntity.Id>builder()
                        .fromKey(
                                new SuiteRestartEntity.Id(iterationId, 0, null, 0, false)
                        )
                        .toKey(
                                new SuiteRestartEntity.Id(
                                        iterationId,
                                        UnsignedLong.MAX_VALUE.longValue(),
                                        String.valueOf(Character.MAX_VALUE),
                                        Integer.MAX_VALUE,
                                        true
                                )
                        )
                        .toInclusive(true)
                        .fromInclusive(true)
                        .ordered()
                        .build()
        ).collect(Collectors.toList());
    }

    public Stream<SuiteRestartEntity.Id> fetchIdsByCheck(CheckEntity.Id checkId, int limit) {
        var fromId = new CheckIterationEntity.Id(checkId, Integer.MIN_VALUE, Integer.MIN_VALUE);
        var toId = new CheckIterationEntity.Id(checkId, Integer.MAX_VALUE, Integer.MAX_VALUE);

        return this.readTableIds(
                ReadTableParams.<SuiteRestartEntity.Id>builder()
                        .fromKey(new SuiteRestartEntity.Id(fromId, Integer.MIN_VALUE, null, 0, false))
                        .toKey(new SuiteRestartEntity.Id(toId, Integer.MAX_VALUE, null, 0, false))
                        .toInclusive(true)
                        .fromInclusive(true)
                        .ordered()
                        .rowLimit(limit)
                        .build()
        );
    }
}
