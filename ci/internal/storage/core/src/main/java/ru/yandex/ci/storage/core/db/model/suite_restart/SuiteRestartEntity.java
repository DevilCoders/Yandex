package ru.yandex.ci.storage.core.db.model.suite_restart;

import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;

@SuppressWarnings({"ReferenceEquality", "BoxedPrimitiveEquality"})
@Value
@Table(name = "SuiteRestarts")
public class SuiteRestartEntity implements Entity<SuiteRestartEntity> {

    Id id;

    String path;

    String jobName;

    @Override
    public Id getId() {
        return id;
    }

    @Value
    public static class Id implements Entity.Id<SuiteRestartEntity> {
        CheckIterationEntity.Id iterationId;

        @Column(dbType = DbType.UINT64)
        long suiteId;

        String toolchain;

        @Column(dbType = DbType.UINT8)
        int partition;

        boolean isRight;
    }
}
