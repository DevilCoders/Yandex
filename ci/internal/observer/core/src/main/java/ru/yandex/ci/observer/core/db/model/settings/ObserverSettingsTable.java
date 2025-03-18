package ru.yandex.ci.observer.core.db.model.settings;

import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;

import ru.yandex.ci.common.ydb.KikimrTableCi;

public class ObserverSettingsTable extends KikimrTableCi<ObserverSettings> {

    public ObserverSettingsTable(QueryExecutor queryExecutor) {
        super(ObserverSettings.class, queryExecutor);
    }

    public void setup() {
        if (!this.exists(ObserverSettings.ID)) {
            this.save(ObserverSettings.EMTPY);
        }
    }
}
