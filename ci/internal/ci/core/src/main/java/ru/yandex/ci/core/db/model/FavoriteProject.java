package ru.yandex.ci.core.db.model;

import javax.annotation.Nonnull;

import lombok.Value;
import lombok.With;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;
import ru.yandex.ci.ydb.Persisted;

@Value
@Table(name = "main/FavoriteProject")
public class FavoriteProject implements Entity<FavoriteProject> {

    @Nonnull
    Id id;

    @Nonnull
    @With
    @Column(dbType = DbType.STRING)
    Mode mode;

    public String getUser() {
        return id.user;
    }

    public String getProject() {
        return id.project;
    }

    @Override
    public Id getId() {
        return id;
    }

    @Persisted
    public enum Mode {
        NONE,
        SET,
        SET_AUTO,
        UNSET
    }

    public static FavoriteProject of(String user, String project, Mode mode) {
        return new FavoriteProject(Id.of(user, project), mode);
    }

    @Value(staticConstructor = "of")
    public static class Id implements Entity.Id<FavoriteProject> {
        @Column(name = "user", dbType = DbType.UTF8)
        String user;
        @Column(name = "project", dbType = DbType.UTF8)
        String project;
    }
}
