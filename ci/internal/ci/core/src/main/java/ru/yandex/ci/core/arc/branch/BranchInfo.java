package ru.yandex.ci.core.arc.branch;


import java.time.Instant;
import java.util.List;

import javax.annotation.Nullable;

import lombok.Builder;
import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;
import ru.yandex.ci.common.ydb.KikimrProjectionCI;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.OrderedArcRevision;

/**
 * Общая информация о ветке, безотносительно релизных процессов.
 * Ветки из этой таблице менеджатся через CI.
 */
@SuppressWarnings("ReferenceEquality")
@Value
@Builder(toBuilder = true)
@Table(name = "main/Branch")
public class BranchInfo implements Entity<BranchInfo>, KikimrProjectionCI {

    @Column(name = "id") // Увы, это уже не изменить
    Id id;

    /**
     * Коммит в транке, от которого отведена ветка.
     */
    @Column(dbType = DbType.UTF8)
    String commitId;

    @Column(dbType = DbType.TIMESTAMP)
    Instant created;

    @Column(dbType = DbType.UTF8)
    String createdBy;

    @Nullable
    @Column(flatten = false)
    OrderedArcRevision baseRevision;

    @Nullable
    @Column(flatten = false)
    OrderedArcRevision configRevision;


    @Override
    public Id getId() {
        return id;
    }

    public ArcBranch getArcBranch() {
        return ArcBranch.ofBranchName(id.getName());
    }

    /**
     * Коммит, от которого отведена ветка
     */
    public OrderedArcRevision getBaseRevision() {
        if (baseRevision == null) {
            // TODO backward compatibility
            return ArcRevision.of(commitId).toOrdered(ArcBranch.trunk(), Integer.MAX_VALUE, 0);
        }
        return baseRevision;
    }

    @Override
    public List<Entity<?>> createProjections() {
        return List.of(BranchInfoByCommitId.of(this));
    }

    @Value(staticConstructor = "of")
    public static class Id implements Entity.Id<BranchInfo> {

        @Column(dbType = DbType.UTF8)
        String name;

        public static BranchInfo.Id of(ArcBranch branch) {
            branch.checkBranchIsReleaseOrUser();
            return of(branch.asString());
        }
    }

    public static class Builder {
        public Builder branch(String name) {
            id(Id.of(name));
            return this;
        }
    }
}
