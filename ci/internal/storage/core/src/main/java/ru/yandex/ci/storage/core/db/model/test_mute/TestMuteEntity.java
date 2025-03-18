package ru.yandex.ci.storage.core.db.model.test_mute;

import java.time.Instant;
import java.util.Objects;

import javax.annotation.Nullable;

import lombok.AccessLevel;
import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.GlobalIndex;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;

import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.test_status.TestStatusEntity;

@Value
@AllArgsConstructor(access = AccessLevel.PRIVATE)
@Table(name = "TestMutes")
@Builder(toBuilder = true)
@GlobalIndex(name = TestMuteEntity.IDX_BY_TIMESTAMP, fields = {"id.timestamp"})
public class TestMuteEntity implements Entity<TestMuteEntity> {
    public static final String IDX_BY_TIMESTAMP = "IDX_BY_TIMESTAMP";
    Id id;

    CheckIterationEntity.Id iterationId;

    String oldSuiteId;
    String oldTestId;
    String name;
    String subtestName;
    String path;

    @Nullable
    Common.ResultType resultType;

    @Nullable
    String service;
    long revisionNumber;

    boolean muted;

    String reason;

    @Override
    public TestMuteEntity.Id getId() {
        return id;
    }

    public Common.ResultType getResultType() {
        return Objects.requireNonNullElse(resultType, Common.ResultType.RT_BUILD);
    }

    @Value
    public static class Id implements Entity.Id<TestMuteEntity> {
        TestStatusEntity.Id testId;

        @Column(dbType = DbType.TIMESTAMP)
        Instant timestamp;
    }
}
