package ru.yandex.ci.core.pr;

import java.time.Instant;
import java.util.List;
import java.util.Objects;

import javax.annotation.Nullable;

import lombok.Value;
import lombok.With;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;

import ru.yandex.arcanum.event.ArcanumModels;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.ydb.Persisted;
import ru.yandex.lang.NonNullApi;

@SuppressWarnings({"ReferenceEquality", "BoxedPrimitiveEquality"})
@Value
@Table(name = "main/PullRequestDiffSet")
public class PullRequestDiffSet implements Entity<PullRequestDiffSet> {

    Id id;

    @With
    @Column(dbType = DbType.UTF8)
    String author;

    @Column(dbType = DbType.UTF8)
    String summary;

    @Nullable
    @Column(dbType = DbType.UTF8)
    String description;

    @Column(flatten = false)
    PullRequestVcsInfo vcsInfo;

    @Nullable
    @Column(flatten = false)
    State state;

    @Column
    List<String> issues;

    @With
    @Nullable
    @Column(dbType = DbType.STRING)
    Status status;

    @With
    @Nullable
    Boolean scheduleCompletion;

    @Nullable
    @Column
    List<String> labels;

    @Column(dbType = DbType.TIMESTAMP)
    @Nullable
    Instant eventCreated;

    @Nullable
    ArcanumModels.DiffSet.Type type;

    public PullRequestDiffSet(
            Id id,
            String author,
            String summary,
            @Nullable String description,
            PullRequestVcsInfo vcsInfo,
            @Nullable State state,
            @Nullable List<String> issues,
            @Nullable Status status,
            @Nullable Boolean scheduleCompletion,
            @Nullable List<String> labels,
            @Nullable Instant eventCreated,
            @Nullable ArcanumModels.DiffSet.Type type
    ) {
        this.id = id;
        this.author = author;
        this.summary = summary;
        this.description = description;
        this.vcsInfo = vcsInfo;
        this.state = state;
        this.issues = issues != null ? issues : List.of();
        this.status = status;
        this.scheduleCompletion = Boolean.TRUE.equals(scheduleCompletion);
        this.labels = labels;
        this.eventCreated = eventCreated;
        this.type = type;
    }

    public PullRequestDiffSet(
            PullRequest pullRequest,
            long diffSetId,
            PullRequestVcsInfo vcsInfo,
            @Nullable State state,
            List<String> issues,
            List<String> labels,
            @Nullable Instant eventCreated,
            @Nullable ArcanumModels.DiffSet.Type type
    ) {
        this(
                Id.of(pullRequest.getId(), diffSetId),
                pullRequest.getAuthor(),
                pullRequest.getSummary(),
                pullRequest.getDescription(),
                vcsInfo,
                state,
                issues,
                Status.NEW,
                false,
                labels,
                eventCreated,
                type
        );
    }

    public ArcanumModels.DiffSet.Type getType() {
        return type == null ? ArcanumModels.DiffSet.Type.DIFF : type;
    }

    public long getDiffSetId() {
        return id.getDiffSetId();
    }

    public long getPullRequestId() {
        return id.getPullRequestId();
    }

    public OrderedArcRevision getOrderedMergeRevision() {
        return OrderedArcRevision.fromRevision(
                vcsInfo.getMergeRevision(),
                ArcBranch.ofPullRequest(getPullRequestId()),
                getDiffSetId(),
                id.pullRequestId
        );
    }

    @Override
    public Id getId() {
        return id;
    }

    public Status getStatus() {
        return status != null ? status : Status.NEW;
    }

    public boolean getScheduleCompletion() {
        return Boolean.TRUE.equals(scheduleCompletion);
    }

    public List<String> getLabels() {
        return Objects.requireNonNullElse(labels, List.of());
    }

    @Persisted
    @Value
    @NonNullApi
    public static class State {
        int affectedConfigs;
    }

    @Value(staticConstructor = "of")
    public static class Id implements Entity.Id<PullRequestDiffSet> {
        @Column(name = "pullRequestId")
        long pullRequestId;
        @Column(name = "diffSetId")
        long diffSetId;
    }

    @Persisted
    public enum Status {
        NEW, // Новый diff-set (DS)
        DISCOVERED, // DS успешно обработан (все необходимые flow подняты, если такие есть)
        COMPLETE, // DS полностью завершен (либо был отменен, либо закоммичен)
        SKIP // DS пропущен (не был поднят на обработку)
    }

    @Value(staticConstructor = "of")
    @Table(name = "main/PullRequestDiffSet_ByPullRequestId")
    public static class ByPullRequestId implements Entity<ByPullRequestId> {

        ByPullRequestId.Id id;

        @Override
        public ByPullRequestId.Id getId() {
            return id;
        }

        public static ByPullRequestId of(PullRequestDiffSet diffSet) {
            return new ByPullRequestId(ByPullRequestId.Id.of(diffSet.getPullRequestId()));
        }

        @Value(staticConstructor = "of")
        public static class Id implements Entity.Id<ByPullRequestId> {
            @Column(name = "idx_pullRequestId")
            long pullRequestId;
        }
    }
}
