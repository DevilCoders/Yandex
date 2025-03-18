package ru.yandex.ci.core.discovery;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.CiProcessId;

@Value
@Table(name = "main/DiscoveredCommit")
public class DiscoveredCommit implements Entity<DiscoveredCommit> {

    @Nonnull
    Id id;

    @Nonnull
    @Column(dbType = DbType.UTF8)
    String commitId;

    @Column
    long pullRequestId;

    @Column
    int stateVersion;

    @Nonnull
    @Column(flatten = false)
    DiscoveredCommitState state;

    @Nullable
    @Column
    Boolean hasCancelledLaunches;


    public CiProcessId getProcessId() {
        return CiProcessId.ofString(id.processId);
    }

    public OrderedArcRevision getArcRevision() {
        return OrderedArcRevision.fromHash(commitId, id.branch, id.commitNumber, pullRequestId);
    }

    public boolean getHasCancelledLaunches() {
        return Boolean.TRUE.equals(hasCancelledLaunches);
    }

    @Nonnull
    @Override
    public Id getId() {
        return id;
    }


    public static DiscoveredCommit of(CiProcessId processId,
                                      OrderedArcRevision arcRevision,
                                      int stateVersion,
                                      DiscoveredCommitState state) {
        return new DiscoveredCommit(
                Id.of(processId.asString(), arcRevision.getBranch().asString(), arcRevision.getNumber()),
                arcRevision.getCommitId(),
                arcRevision.getPullRequestId(),
                stateVersion,
                state,
                !state.getCancelledLaunchIds().isEmpty());
    }

    @Value(staticConstructor = "of")
    public static class Id implements Entity.Id<DiscoveredCommit> {
        @Column(name = "processId", dbType = DbType.UTF8)
        String processId;

        @Column(name = "branch", dbType = DbType.UTF8)
        String branch;

        @Column(name = "commitNumber")
        long commitNumber;

        public static Id of(CiProcessId processId, OrderedArcRevision revision) {
            return Id.of(processId.asString(), revision.getBranch().asString(), revision.getNumber());
        }

    }

}
