package ru.yandex.ci.storage.core.db.model.check_task;

import java.util.Objects;

import javax.annotation.Nullable;

import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Value;
import lombok.With;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;

import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.check.CheckTaskTypeUtils;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.test.TestEntity;

@SuppressWarnings({"ReferenceEquality", "BoxedPrimitiveEquality"})
@Value
@Builder(toBuilder = true)
@Table(name = "LargeTasks")
public class LargeTaskEntity implements Entity<LargeTaskEntity> {

    Id id;

    @Nullable
    String leftTaskId;

    @Nullable
    String rightTaskId;

    // Single path for normal Large test, multiple paths separated by comma for native builds
    // Native builds means id.checkTaskType == CTT_NATIVE_BUILD
    String target;

    // Configured only for native builds
    @Nullable
    String nativeTarget;

    @Nullable
    NativeSpecification nativeSpecification;

    @Nullable
    @Column(name = "largeTestInfo", flatten = false, dbType = DbType.JSON)
    LargeTestInfo leftLargeTestInfo;

    @Nullable
    @Column(flatten = false, dbType = DbType.JSON)
    LargeTestInfo rightLargeTestInfo;

    @Nullable
    String startedBy;

    @Nullable
    @Column(flatten = false, dbType = DbType.JSON)
    LargeTestDelegatedConfig delegatedConfig;

    @Nullable
    @Column(flatten = false, dbType = DbType.JSON)
    OrderedArcRevision configRevision;

    // Flow launch number, null during creation, not null only after actual launch
    @With
    @Nullable
    Integer launchNumber;

    @Override
    public Id getId() {
        return id;
    }

    @Nullable
    public CheckTaskEntity.Id toLeftTaskId() {
        return leftTaskId == null
                ? null
                : new CheckTaskEntity.Id(id.iterationId, leftTaskId);
    }

    @Nullable
    public CheckTaskEntity.Id toRightTaskId() {
        return rightTaskId == null
                ? null
                : new CheckTaskEntity.Id(id.iterationId, rightTaskId);
    }

    public CiProcessId toCiProcessId() {
        var testInfo = getLeftLargeTestInfo();
        return CheckTaskTypeUtils.toCiProcessId(
                id.getCheckTaskType(),
                target,
                nativeTarget,
                testInfo.getToolchain(),
                testInfo.getSuiteName()
        );
    }

    public TestEntity.Id toTestId() {
        var testInfo = getLeftLargeTestInfo();
        return TestEntity.Id.of(testInfo.getSuiteHid().longValue(), testInfo.getToolchain());
    }

    public LargeTestInfo getLeftLargeTestInfo() {
        if (leftLargeTestInfo != null) {
            return leftLargeTestInfo;
        }
        return Objects.requireNonNull(rightLargeTestInfo,
                "Either leftLargeTestInfo or rightLargeTestInfo must be not null, but both are null");
    }

    public LargeTestInfo getRightLargeTestInfo() {
        if (rightLargeTestInfo != null) {
            return rightLargeTestInfo;
        }
        return Objects.requireNonNull(leftLargeTestInfo,
                "Either leftLargeTestInfo or rightLargeTestInfo must be not null, but both are null");
    }

    @Value
    @AllArgsConstructor
    public static class Id implements Entity.Id<LargeTaskEntity> {
        CheckIterationEntity.Id iterationId;
        Common.CheckTaskType checkTaskType;

        // Some unique index within single iteration
        int index;

        @Override
        public String toString() {
            return "[" + iterationId + "/" + checkTaskType + "/" + index + "]";
        }
    }

}
