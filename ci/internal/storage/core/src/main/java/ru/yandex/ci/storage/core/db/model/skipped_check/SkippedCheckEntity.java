package ru.yandex.ci.storage.core.db.model.skipped_check;

import java.time.Instant;

import lombok.AllArgsConstructor;
import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;

@Value
@AllArgsConstructor
@Table(name = "SkippedChecks")
public class SkippedCheckEntity implements Entity<SkippedCheckEntity> {

    Id id;

    @Column(dbType = DbType.TIMESTAMP)
    Instant created;

    String reason;

    public SkippedCheckEntity(Id id, String reason) {
        this.id = id;
        this.created = Instant.now();
        this.reason = reason;
    }

    @Override
    public Id getId() {
        return id;
    }

    @Value
    public static class Id implements Entity.Id<SkippedCheckEntity> {
        Long id;

        public static Id of(CheckEntity.Id checkId) {
            return new Id(checkId.getId());
        }
    }
}
