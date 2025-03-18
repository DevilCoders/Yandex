package ru.yandex.ci.core.launch.versioning;

import java.util.List;
import java.util.Objects;

import lombok.Builder;
import lombok.Value;
import lombok.With;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.CiProcessId;

@SuppressWarnings("ReferenceEquality")
@Value
@Builder(toBuilder = true)
@Table(name = "main/Versions")
public class Versions implements Entity<Versions> {

    @With
    Id id;

    @Column(flatten = false)
    List<Slot> slots;

    @Override
    public Id getId() {
        return id;
    }

    public List<Slot> getSlots() {
        return Objects.requireNonNullElse(slots, List.of());
    }

    @SuppressWarnings("ReferenceEquality")
    @Value(staticConstructor = "of")
    public static class Id implements Entity.Id<Versions> {
        @With
        @Column(name = "processId", dbType = DbType.UTF8)
        String processId;

        @Column(name = "branch", dbType = DbType.UTF8)
        String branch;

        @Column(name = "commitNumber")
        long commitNumber;

        public static Versions.Id of(CiProcessId processId, OrderedArcRevision revision) {
            return Versions.Id.of(processId.asString(), revision.getBranch().asString(), revision.getNumber());
        }

    }
}
