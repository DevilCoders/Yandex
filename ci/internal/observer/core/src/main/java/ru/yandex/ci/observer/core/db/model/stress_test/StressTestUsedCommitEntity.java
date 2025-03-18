package ru.yandex.ci.observer.core.db.model.stress_test;

import javax.annotation.Nonnull;

import lombok.Builder;
import lombok.Getter;
import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.GlobalIndex;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;

@Value
@Builder(toBuilder = true)
@Table(name = "StressTestUsedCommit")
@GlobalIndex(name = StressTestUsedCommitEntity.IDX_NAMESPACE_COMMIT_NUMBER,
        fields = {"id.namespace", "id.leftRevisionNumber"})
public class StressTestUsedCommitEntity implements Entity<StressTestUsedCommitEntity> {

    public static final String IDX_NAMESPACE_COMMIT_NUMBER = "IDX_NAMESPACE_COMMIT_NUMBER";

    @Nonnull
    @Getter(onMethod_ = @Override)
    Id id;

    @Nonnull
    String flowLaunchId;

    @Value(staticConstructor = "of")
    public static class Id implements Entity.Id<StressTestUsedCommitEntity> {

        @Nonnull
        String rightCommitId;

        @Nonnull
        String namespace;

        @Column
        long leftRevisionNumber;

    }

}
