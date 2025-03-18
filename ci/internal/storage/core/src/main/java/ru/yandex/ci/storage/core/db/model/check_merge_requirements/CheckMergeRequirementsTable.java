package ru.yandex.ci.storage.core.db.model.check_merge_requirements;

import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;

import ru.yandex.ci.common.ydb.KikimrTableCi;

public class CheckMergeRequirementsTable extends KikimrTableCi<CheckMergeRequirementsEntity> {
    public CheckMergeRequirementsTable(QueryExecutor executor) {
        super(CheckMergeRequirementsEntity.class, executor);
    }

}
