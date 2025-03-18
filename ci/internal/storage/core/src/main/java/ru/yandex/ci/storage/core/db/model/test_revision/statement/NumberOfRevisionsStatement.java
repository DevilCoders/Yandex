package ru.yandex.ci.storage.core.db.model.test_revision.statement;

import java.util.Map;
import java.util.stream.Collectors;

import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.repository.db.EntitySchema;
import yandex.cloud.repository.db.ViewSchema;
import yandex.cloud.repository.db.YqlVersion;
import yandex.cloud.repository.kikimr.DbType;
import yandex.cloud.repository.kikimr.statement.YqlStatement;

import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.model.test.TestEntity;
import ru.yandex.ci.storage.core.db.model.test_revision.HistoryFilters;
import ru.yandex.ci.storage.core.db.model.test_revision.TestRevisionEntity;
import ru.yandex.ci.storage.core.db.model.test_revision.WrappedRevisionsBoundaries;
import ru.yandex.ci.storage.core.db.model.test_status.TestStatusEntity;

public class NumberOfRevisionsStatement
        extends YqlStatement<Object, TestRevisionEntity, NumberOfRevisionsStatement.NumberOfRevisions> {
    private final TestStatusEntity.Id testId;
    private final HistoryFilters filters;
    private final Map<String, WrappedRevisionsBoundaries> boundaries;

    public NumberOfRevisionsStatement(
            TestStatusEntity.Id testId,
            HistoryFilters filters,
            Map<String, WrappedRevisionsBoundaries> boundaries
    ) {
        super(
                EntitySchema.of(TestRevisionEntity.class),
                ViewSchema.of(NumberOfRevisionsStatement.NumberOfRevisions.class)
        );
        this.testId = testId;
        this.filters = filters;
        this.boundaries = boundaries;
    }

    @Override
    public String getQuery(String tablespace, YqlVersion version) {
        var declarations = this.declarations(version);
        var table = this.table(tablespace);

        var testFilter = "id_statusId_testId=%d AND id_statusId_suiteId=%d AND id_statusId_branch='%s' ".formatted(
                testId.getTestId(), testId.getSuiteId(), testId.getBranch()
        );

        var status = filters.getStatus();
        if (!status.equals(Common.TestStatus.UNRECOGNIZED) && !status.equals(Common.TestStatus.TS_UNKNOWN)) {
            testFilter = testFilter + " AND (previousStatus = %s OR status = %s) ".formatted(status, status);
        }

        if (!testId.getToolchain().isEmpty() && !testId.getToolchain().equals(TestEntity.ALL_TOOLCHAINS)) {
            testFilter = testFilter + " AND id_statusId_toolchain='%s'".formatted(testId.getToolchain());
        }

        var template =
                "SELECT '%s'u as boundary, count(distinct id_revision) as number\n" +
                        "FROM " + table + "\n" +
                        "WHERE " + testFilter + " AND" + " id_revision > %d and id_revision < %d";

        var suitesQueries = boundaries.entrySet().stream()
                .map(b -> template.formatted(b.getKey(), b.getValue().getTo(), b.getValue().getFrom()))
                .collect(Collectors.joining("\nUNION ALL\n"));

        return "%s\n%s".formatted(declarations, suitesQueries);
    }

    @Override
    public QueryType getQueryType() {
        return QueryType.SELECT;
    }

    @Override
    public String toDebugString(Object params) {
        return "NumberOfRevisionsStatement";
    }

    @Value
    public static class NumberOfRevisions implements yandex.cloud.repository.db.Table.View {
        @Column(name = "boundary", dbType = DbType.UTF8)
        String boundary;

        @Column(name = "number", dbType = DbType.UINT64)
        Long number;
    }
}
