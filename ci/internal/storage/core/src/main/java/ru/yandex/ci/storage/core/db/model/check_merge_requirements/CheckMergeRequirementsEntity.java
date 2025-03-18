package ru.yandex.ci.storage.core.db.model.check_merge_requirements;

import java.time.Instant;
import java.util.List;
import java.util.Objects;

import javax.annotation.Nullable;

import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;

import ru.yandex.ci.client.arcanum.ArcanumMergeRequirementDto;
import ru.yandex.ci.core.pr.ArcanumCheckType;
import ru.yandex.ci.storage.core.check.ArcanumCheckStatus;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;

@Value
@Builder(toBuilder = true)
@AllArgsConstructor
@Table(name = "CheckMergeRequirements")
public class CheckMergeRequirementsEntity implements Entity<CheckMergeRequirementsEntity> {

    Id id;

    ArcanumCheckStatus value;

    @Nullable
    List<MergeInterval> mergeIntervals;

    @Column(dbType = DbType.TIMESTAMP)
    Instant updatedAt;

    @Nullable
    @Column(dbType = DbType.TIMESTAMP)
    Instant reportedAt;

    @Override
    public Id getId() {
        return id;
    }

    public ArcanumMergeRequirementDto.Status getStatus() {
        return value.getStatus();
    }

    @Value
    public static class Id implements Entity.Id<CheckMergeRequirementsEntity> {
        CheckEntity.Id checkId;
        ArcanumCheckType arcanumCheckType;
    }

    public List<MergeInterval> getMergeIntervals() {
        return Objects.requireNonNullElse(mergeIntervals, List.of());
    }
}
