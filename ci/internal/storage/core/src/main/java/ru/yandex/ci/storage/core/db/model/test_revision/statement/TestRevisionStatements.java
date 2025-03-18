package ru.yandex.ci.storage.core.db.model.test_revision.statement;

import java.util.Collection;
import java.util.List;

import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.repository.db.EntitySchema;
import yandex.cloud.repository.db.Table;
import yandex.cloud.repository.db.ViewSchema;
import yandex.cloud.repository.db.YqlVersion;
import yandex.cloud.repository.kikimr.DbType;
import yandex.cloud.repository.kikimr.statement.PredicateStatement;
import yandex.cloud.repository.kikimr.statement.Statement;
import yandex.cloud.repository.kikimr.yql.YqlStatementPart;

import ru.yandex.ci.common.ydb.YqlStatementCi;
import ru.yandex.ci.storage.core.db.model.test_revision.TestRevisionEntity;

public class TestRevisionStatements {
    private static final EntitySchema<TestRevisionEntity> SCHEMA = EntitySchema.of(TestRevisionEntity.class);

    private TestRevisionStatements() {

    }

    public static Statement<Collection<? extends YqlStatementPart<?>>, IdRevision> createRegionsStatement(
            YqlStatementPart<?> filter,
            int regionsSize
    ) {
        var outSchema = ViewSchema.of(IdRevision.class);
        return new PredicateStatement<>(SCHEMA, outSchema, List.of(filter), YqlStatementCi::predicateFrom) {
            @Override
            public String getQuery(String tablespace, YqlVersion version) {
                var paramNames = this.resolveParamNames(filter.toYql(schema));

                return declarations(version) + "\n" +
                        "SELECT DISTINCT(id_revision / " + regionsSize + ") as revision\n" +
                        "FROM " + table(tablespace) + "\n" +
                        "WHERE " + paramNames + "\n";
            }

            @Override
            public QueryType getQueryType() {
                return QueryType.SELECT;
            }

            @Override
            public String toDebugString(Collection<? extends YqlStatementPart<?>> yqlStatementParts) {
                return "regions(" + yqlStatementParts + ")";
            }
        };
    }

    public static Statement<Collection<? extends YqlStatementPart<?>>, IdRevision> createBucketsStatement(
            YqlStatementPart<?> filter,
            int regionOffset,
            int bucketSize
    ) {
        var outSchema = ViewSchema.of(IdRevision.class);
        return new PredicateStatement<>(SCHEMA, outSchema, List.of(filter), YqlStatementCi::predicateFrom) {
            @Override
            public String getQuery(String tablespace, YqlVersion version) {
                var paramNames = this.resolveParamNames(filter.toYql(schema));

                return declarations(version) + "\n" +
                        "SELECT DISTINCT((id_revision - " + regionOffset + ") / " + bucketSize + ") as revision\n" +
                        "FROM " + table(tablespace) + "\n" +
                        "WHERE " + paramNames + "\n";
            }

            @Override
            public QueryType getQueryType() {
                return QueryType.SELECT;
            }

            @Override
            public String toDebugString(Collection<? extends YqlStatementPart<?>> yqlStatementParts) {
                return "buckets(" + yqlStatementParts + ")";
            }
        };
    }

    @Value
    public static class IdRevision implements Table.View {
        @Column(name = "revision", dbType = DbType.INT64)
        long revision;

    }
}

