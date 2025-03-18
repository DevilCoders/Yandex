package ru.yandex.ci.core.timeline;

import java.time.Instant;
import java.util.Objects;

import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import lombok.Builder;
import lombok.Value;
import lombok.With;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchState;

@Value
@Builder(toBuilder = true)
@Table(name = "main/Timeline")
public class TimelineItemEntity implements Entity<TimelineItemEntity> {

    TimelineItemEntity.Id id;

    @Column(flatten = false, name = "launchRef")
    @Nullable
    Launch.Id launch;

    @Column(flatten = false, name = "branchRef")
    @Nullable
    TimelineBranchItem.Id branch;

    @Column
    boolean hidden;

    /**
     * Релиз может быть создан фактически в одной ветке (trunk), но отображаться в релизной ветке.
     * Это поле отображает как раз визуальное расположение timeline элемента.
     */
    @Column(dbType = DbType.UTF8)
    @Nullable
    String showInBranch;

    @Column
    long timelineVersion;

    @Nullable
    @Column(dbType = DbType.UTF8)
    LaunchState.Status status;

    @Nullable
    @Column(dbType = DbType.TIMESTAMP)
    Instant started;

    @Nullable
    @Column(dbType = DbType.TIMESTAMP)
    Instant finished;

    @Column(name = "launchNumber")
    Integer launchNumber;

    @Nullable
    Boolean inStable;

    private TimelineItemEntity(Id id,
                               @Nullable Launch.Id launch,
                               @Nullable TimelineBranchItem.Id branch,
                               boolean hidden,
                               @Nullable String showInBranch,
                               long timelineVersion,
                               @Nullable LaunchState.Status status,
                               @Nullable Instant started,
                               @Nullable Instant finished,
                               @Nullable Integer launchNumber,
                               @Nullable Boolean inStable) {

        Preconditions.checkArgument(
                (launch == null) ^ (branch == null),
                "Should have either launch or branch, got %s %s for id %s",
                launch, branch, id
        );

        this.id = id;
        this.launch = launch;
        this.branch = branch;
        this.hidden = hidden;
        this.showInBranch = showInBranch;
        this.timelineVersion = timelineVersion;
        this.status = status;
        this.started = started;
        this.finished = finished;
        this.launchNumber = launchNumber;
        this.inStable = inStable;
    }

    @Override
    public TimelineItemEntity.Id getId() {
        return id;
    }

    public CiProcessId getProcessId() {
        return CiProcessId.ofString(id.getProcessId());
    }

    public boolean getInStable() {
        return Objects.requireNonNullElse(inStable, false);
    }

    public static class Builder {
        public Builder branchId(CiProcessId processId, String branchName) {
            branch(TimelineBranchItem.Id.of(processId.asString(), branchName));
            return this;
        }

        public Builder launch(@Nullable Launch.Id launch) {
            this.launch = launch;
            this.launchNumber = launch != null
                    ? launch.getLaunchNumber()
                    : null;
            return this;
        }
    }

    @SuppressWarnings("ReferenceEquality")
    @Value
    public static class Id implements Entity.Id<TimelineItemEntity> {
        @With
        @Column(name = "processId", dbType = DbType.UTF8)
        String processId;

        @Column(name = "branch", dbType = DbType.UTF8)
        String branch;

        @Column(name = "revision")
        long revision;

        @Column(name = "itemNumber")
        int itemNumber;

        public static Id of(CiProcessId processId, OrderedArcRevision revision, int itemNumber) {
            return new Id(processId.asString(), revision.getBranch().asString(), revision.getNumber(), itemNumber);
        }

        public static Id of(CiProcessId processId, ArcBranch branch, long revision, int itemNumber) {
            return new Id(processId.asString(), branch.asString(), revision, itemNumber);
        }
    }
}
