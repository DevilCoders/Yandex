package ru.yandex.ci.storage.core.db.model.test_mute;

import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;

import ru.yandex.ci.common.ydb.KikimrTableCi;

public class OldCiMuteActionTable extends KikimrTableCi<OldCiMuteActionEntity> {

    public OldCiMuteActionTable(QueryExecutor executor) {
        super(OldCiMuteActionEntity.class, executor);
    }


}
