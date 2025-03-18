package ru.yandex.ci.core.launch;

import java.time.Instant;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.Builder;
import lombok.Value;
import lombok.With;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.VirtualCiProcessId.VirtualType;
import ru.yandex.ci.ydb.Persisted;

@Table(name = "main/PostponeLaunch")
@Value
@Builder
public class PostponeLaunch implements Entity<PostponeLaunch> {

    @Nonnull
    PostponeLaunch.Id id;

    @Nonnull
    @Column(dbType = DbType.TIMESTAMP)
    Instant commitTime;

    int launchNumber;

    @Nullable
    @Column(dbType = DbType.STRING)
    VirtualType virtualType;

    @With
    @Nonnull
    @Column(dbType = DbType.STRING)
    PostponeStatus status;

    @With
    @Nullable
    @Column(dbType = DbType.STRING)
    StartReason startReason;

    @With
    @Nullable
    @Column(dbType = DbType.STRING)
    StopReason stopReason;

    @Override
    public PostponeLaunch.Id getId() {
        return id;
    }

    public Launch.Id toLaunchId() {
        return Launch.Id.of(id.processId, launchNumber);
    }

    public long getSvnRevision() {
        return id.getSvnRevision();
    }

    @SuppressWarnings("ReferenceEquality")
    @Value(staticConstructor = "of")
    public static class Id implements Entity.Id<PostponeLaunch> {
        @Nonnull
        @With
        @Column(name = "processId", dbType = DbType.UTF8)
        String processId;

        @Nonnull
        @Column(name = "branch", dbType = DbType.UTF8)
        String branch;

        @Column(name = "svnRevision")
        long svnRevision;

        public static Id of(CiProcessId ciProcessId, long svnRevision) {
            return of(ciProcessId.toString(), ArcBranch.trunk().getBranch(), svnRevision);
        }
    }

    @Persisted
    public enum PostponeStatus {
        NEW,
        START_SCHEDULED,
        STARTED,
        SKIPPED,
        COMPLETE,
        CANCELED,
        FAILURE;

        public boolean isRunning() {
            return this == START_SCHEDULED || this == STARTED;
        }

        public boolean isExecutionComplete() {
            return this == COMPLETE || this == CANCELED || this == FAILURE;
        }
    }

    @Persisted
    public enum StartReason {
        DEFAULT, BINARY_SEARCH
    }

    @Persisted
    public enum StopReason {
        OBSOLETE_LAUNCH
    }
}

