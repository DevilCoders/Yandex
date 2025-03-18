package ru.yandex.ci.common.ydb;

import javax.annotation.Nonnull;

import lombok.AllArgsConstructor;

import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.db.EntitySchema;
import yandex.cloud.repository.kikimr.yql.YqlOrderBy;
import yandex.cloud.repository.kikimr.yql.YqlStatementPart;

@AllArgsConstructor
public class YqlRawOrderBy implements YqlStatementPart<YqlRawOrderBy> {
    @Nonnull
    private final String yql;

    @Override
    public String getType() {
        return YqlOrderBy.TYPE;
    }

    @Override
    public String getYqlPrefix() {
        return "ORDER BY ";
    }

    @Override
    public int getPriority() {
        return 100;
    }

    @Override
    public <T extends Entity<T>> String toYql(@Nonnull EntitySchema<T> entitySchema) {
        return yql;
    }

    @Override
    public String toString() {
        return getYqlPrefix() + yql;
    }
}
