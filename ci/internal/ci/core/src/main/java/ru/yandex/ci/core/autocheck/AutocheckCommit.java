package ru.yandex.ci.core.autocheck;

import lombok.Getter;
import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;
import ru.yandex.ci.ydb.Persisted;

@Value(staticConstructor = "of")
@Table(name = "main/AutocheckCommit")
public class AutocheckCommit implements Entity<AutocheckCommit> {

    @Getter(onMethod_ = @Override)
    Id id;

    @Column(dbType = DbType.UTF8)
    Status status;

    @Value(staticConstructor = "of")
    public static class Id implements Entity.Id<AutocheckCommit> {
        @Column(dbType = DbType.UTF8)
        String commitId;
    }

    @Persisted
    public enum Status {
        IGNORED,    // robot's commits can be ignored
        CHECKING
    }

}
