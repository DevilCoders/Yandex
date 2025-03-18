package ru.yandex.ci.core.security;

import java.time.Instant;

import lombok.Builder;
import lombok.ToString;
import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.GlobalIndex;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;
import ru.yandex.ci.core.arc.ArcRevision;

@Value
@Builder(toBuilder = true)
@Table(name = "security/YavToken")
@GlobalIndex(name = YavToken.IDX_PATH_COMMIT, fields = {"configPath", "delegationCommitId"})
public class YavToken implements Entity<YavToken> {
    public static final String IDX_PATH_COMMIT = "IDX_PATH_COMMIT";

    @Column(name = "uid")
    YavToken.Id id;

    @Column(dbType = DbType.UTF8)
    String configPath;

    @Column(dbType = DbType.UTF8)
    String secretUuid;

    @ToString.Exclude
    @Column(dbType = DbType.UTF8)
    String token; // Delegated token we should use to access the actual values in YAV

    @Column(dbType = DbType.UTF8)
    String delegationCommitId;

    @Column(dbType = DbType.UTF8)
    String delegatedBy;

    @Column(dbType = DbType.UTF8)
    String abcService;

    @Column(dbType = DbType.TIMESTAMP)
    Instant created;

    @Override
    public YavToken.Id getId() {
        return id;
    }

    public ArcRevision getRevision() {
        return ArcRevision.of(delegationCommitId);
    }

    @Value(staticConstructor = "of")
    public static class Id implements Entity.Id<YavToken> {
        @Column(dbType = DbType.UTF8)
        String id;
    }
}
