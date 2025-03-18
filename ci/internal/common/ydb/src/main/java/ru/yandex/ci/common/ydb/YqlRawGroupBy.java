package ru.yandex.ci.common.ydb;

import javax.annotation.Nonnull;

import lombok.AllArgsConstructor;

import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.db.EntitySchema;
import yandex.cloud.repository.kikimr.yql.YqlStatementPart;

@AllArgsConstructor
public class YqlRawGroupBy implements YqlStatementPart<YqlRawGroupBy> {

    @Nonnull
    private final String yql;

    @Override
    public String getType() {
        return "GroupBy";
    }

    @Override
    public int getPriority() {
        return 99; // Before OrderBy
    }

    @Override
    public String getYqlPrefix() {
        return "GROUP BY ";
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
