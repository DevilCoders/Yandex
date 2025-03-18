package ru.yandex.ci.storage.core.db.model.settings;

import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;

import ru.yandex.ci.common.ydb.KikimrTableCi;

public class SettingsTable extends KikimrTableCi<SettingsEntity> {

    public SettingsTable(QueryExecutor queryExecutor) {
        super(SettingsEntity.class, queryExecutor);
    }

    public void setup() {
        if (!this.exists(SettingsEntity.ID)) {
            this.save(SettingsEntity.EMTPY);
        }
    }
}
