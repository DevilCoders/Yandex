package ru.yandex.ci.core.pr;

import java.util.Objects;

import com.google.common.base.Preconditions;
import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.OrderedArcRevision;

@Value
@Table(name = "main/CommitNumber")
public class RevisionNumber implements Entity<RevisionNumber> {

    Id id;

    @Column
    long number;

    @Column
    long pullRequestId;

    public RevisionNumber(Id id, long number, Long pullRequestId) {
        Preconditions.checkArgument(number > 0, "Number cannot be less or equal zero for commit %s", id);
        this.id = id;
        this.number = number;
        this.pullRequestId = Objects.requireNonNullElse(pullRequestId, 0L);
    }

    @Override
    public Id getId() {
        return id;
    }

    public OrderedArcRevision toOrderedArcRevision() {
        return OrderedArcRevision.fromRevision(id.getArcRevision(), id.getBranch(), number, pullRequestId);
    }

    @Value(staticConstructor = "of")
    public static class Id implements Entity.Id<RevisionNumber> {
        @Column(name = "branch", dbType = DbType.UTF8)
        String branch;
        @Column(name = "commitId", dbType = DbType.UTF8)
        String commitId;

        public ArcRevision getArcRevision() {
            return ArcRevision.of(commitId);
        }
    }
}
