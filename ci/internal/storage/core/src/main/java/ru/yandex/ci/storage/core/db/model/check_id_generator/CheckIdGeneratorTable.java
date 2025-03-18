package ru.yandex.ci.storage.core.db.model.check_id_generator;

import javax.annotation.Nullable;

import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;
import yandex.cloud.repository.kikimr.yql.YqlLimit;
import yandex.cloud.repository.kikimr.yql.YqlOrderBy;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;

import ru.yandex.ci.common.ydb.KikimrTableCi;

public class CheckIdGeneratorTable extends KikimrTableCi<CheckIdGeneratorEntity> {
    public CheckIdGeneratorTable(QueryExecutor executor) {
        super(CheckIdGeneratorEntity.class, executor);
    }


    @Nullable
    public Long getId() {
        var ids = this.find(YqlPredicate.alwaysTrue(), YqlOrderBy.orderBy("id"), YqlLimit.range(0, 1));
        if (ids.isEmpty()) {
            return null;
        }

        this.delete(ids.get(0).getId());
        return ids.get(0).getValue();
    }
}
