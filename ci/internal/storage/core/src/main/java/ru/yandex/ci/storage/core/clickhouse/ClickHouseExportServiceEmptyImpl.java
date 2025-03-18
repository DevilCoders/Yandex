package ru.yandex.ci.storage.core.clickhouse;

import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;

public class ClickHouseExportServiceEmptyImpl implements ClickHouseExportService {
    @Override
    public void process(CheckIterationEntity.Id iterationId) {

    }
}
