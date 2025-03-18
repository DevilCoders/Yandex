package ru.yandex.ci.core.db.model;

import java.util.Objects;
import java.util.Set;

import javax.annotation.Nullable;

import lombok.Value;
import lombok.With;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.GlobalIndex;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.discovery.DiscoveryType;
import ru.yandex.ci.ydb.Persisted;

@Value
@Table(name = "main/AutoReleaseQueueItem")
@GlobalIndex(name = AutoReleaseQueueItem.IDX_BY_STATE, fields = {"state"})
@GlobalIndex(name = AutoReleaseQueueItem.IDX_BY_PROCESS_ID_AND_STATE, fields = {"id.processId", "state"})
public class AutoReleaseQueueItem implements Entity<AutoReleaseQueueItem> {

    public static final String IDX_BY_STATE = "IDX_BY_STATE";
    public static final String IDX_BY_PROCESS_ID_AND_STATE = "IDX_BY_PROCESS_ID_AND_STATE";

    AutoReleaseQueueItem.Id id;
    @Column(dbType = DbType.UTF8)
    @With
    State state;

    @Column(flatten = false)
    OrderedArcRevision orderedArcRevision;

    // nullable for old values
    @Nullable
    @Column(dbType = DbType.JSON)
    Set<DiscoveryType> requiredDiscoveries;

    @Override
    public Id getId() {
        return id;
    }

    public Set<DiscoveryType> getRequiredDiscoveries() {
        return Objects.requireNonNullElseGet(requiredDiscoveries, () -> Set.of(DiscoveryType.DIR));
    }

    public static AutoReleaseQueueItem of(OrderedArcRevision revision, CiProcessId processId, State state) {
        return AutoReleaseQueueItem.of(revision, processId, state, Set.of(DiscoveryType.DIR));
    }

    public static AutoReleaseQueueItem of(
            OrderedArcRevision revision,
            CiProcessId processId,
            State state,
            Set<DiscoveryType> requiredDiscoveries
    ) {
        var id = Id.of(revision.getCommitId(), processId.asString());
        return new AutoReleaseQueueItem(id, state, revision, requiredDiscoveries);
    }


    @Persisted
    public enum State {
        WAITING_PREVIOUS_COMMITS,
        WAITING_CONDITIONS,
        WAITING_SCHEDULE,
        // means that we passively waiting when someone will update status WAITING_FREE_STAGE -> CHECKING_FREE_STAGE
        WAITING_FREE_STAGE,
        // means that we are actively checking that stage is free
        CHECKING_FREE_STAGE,
    }

    @Value(staticConstructor = "of")
    public static class Id implements Entity.Id<AutoReleaseQueueItem> {

        @Column(name = "commitId", dbType = DbType.UTF8)
        String commitId;

        @Column(name = "processId", dbType = DbType.UTF8)
        String processId;

    }

}
