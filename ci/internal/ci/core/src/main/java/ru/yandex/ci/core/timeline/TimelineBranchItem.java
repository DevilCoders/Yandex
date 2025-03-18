package ru.yandex.ci.core.timeline;

import java.util.List;

import javax.annotation.Nullable;

import lombok.Builder;
import lombok.Value;
import lombok.With;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.GlobalIndex;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;

import ru.yandex.ci.common.ydb.KikimrProjectionCI;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.branch.BranchInfo;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.launch.versioning.Version;

/**
 * Информация о ветке, которая отображается в таймлайне релизного процесса.
 */
@SuppressWarnings("ReferenceEquality")
@Value
@Builder(toBuilder = true)
@Table(name = "main/BranchItem")
@GlobalIndex(name = TimelineBranchItem.IDX_BRANCH, fields = "id.branch")
public class TimelineBranchItem implements Entity<TimelineBranchItem>, KikimrProjectionCI {
    public static final String IDX_BRANCH = "IDX_BRANCH";

    @With
    Id id;

    /**
     * Версия в строковом варианте. Сейчас используется структурированная версия
     *
     * @deprecated use version
     */
    @Column(dbType = DbType.UTF8, name = "version")
    @Deprecated
    String versionString;

    @Column(dbType = DbType.JSON, flatten = false)
    BranchVcsInfo vcsInfo;

    @Column(dbType = DbType.JSON, flatten = false)
    BranchState state;

    @Nullable
    @Column(dbType = DbType.JSON, flatten = false, name = "ver")
    Version version;

    public Version getVersion() {
        if (version == null) {
            return Version.fromAsString(versionString);
        }
        return version;
    }

    @Override
    public Id getId() {
        return id;
    }

    public CiProcessId getProcessId() {
        return CiProcessId.ofString(id.getProcessId());
    }

    public ArcBranch getArcBranch() {
        return ArcBranch.ofBranchName(id.getBranch());
    }

    @Override
    public List<Entity<?>> createProjections() {
        return List.of(TimelineBranchItemByUpdateDate.of(this));
    }

    @SuppressWarnings("ReferenceEquality")
    @Value(staticConstructor = "of")
    public static class Id implements Entity.Id<TimelineBranchItem> {
        @With
        @Column(name = "processId", dbType = DbType.UTF8)
        String processId;

        @Column(name = "branch", dbType = DbType.UTF8)
        String branch;

        public static Id of(CiProcessId processId, ArcBranch branch) {
            branch.checkBranchIsReleaseOrUser();
            return of(processId.asString(), branch.asString());
        }

        public BranchInfo.Id getInfoId() {
            return BranchInfo.Id.of(branch);
        }
    }

    public static class Builder {
        public TimelineBranchItem.Builder idOf(CiProcessId processId, ArcBranch branch) {
            id(Id.of(processId, branch));
            return this;
        }
    }

}
