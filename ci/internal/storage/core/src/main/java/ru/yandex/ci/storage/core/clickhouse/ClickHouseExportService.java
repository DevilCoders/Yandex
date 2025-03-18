package ru.yandex.ci.storage.core.clickhouse;

import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;

public interface ClickHouseExportService {
    void process(CheckIterationEntity.Id iterationId);
}
