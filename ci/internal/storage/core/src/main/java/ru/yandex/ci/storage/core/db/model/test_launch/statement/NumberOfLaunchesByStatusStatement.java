package ru.yandex.ci.storage.core.db.model.test_launch.statement;

import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.repository.db.EntitySchema;
import yandex.cloud.repository.db.ViewSchema;
import yandex.cloud.repository.db.YqlVersion;
import yandex.cloud.repository.kikimr.DbType;
import yandex.cloud.repository.kikimr.statement.YqlStatement;

import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.model.test_launch.TestLaunchEntity;
import ru.yandex.ci.storage.core.db.model.test_status.TestStatusEntity;

public class NumberOfLaunchesByStatusStatement
        extends YqlStatement<Object, TestLaunchEntity, NumberOfLaunchesByStatusStatement.Result> {
    private final TestStatusEntity.Id testId;
    private final long revision;

    public NumberOfLaunchesByStatusStatement(
            TestStatusEntity.Id testId,
            long revision
    ) {
        super(
                EntitySchema.of(TestLaunchEntity.class),
                ViewSchema.of(NumberOfLaunchesByStatusStatement.Result.class)
        );
        this.testId = testId;
        this.revision = revision;
    }

    @Override
    public String getQuery(String tablespace, YqlVersion version) {
        var declarations = this.declarations(version);
        var table = this.table(tablespace);

        var filter = (
                "id_statusId_testId=%d " +
                        "AND id_statusId_suiteId=%d " +
                        "AND id_statusId_branch='%s' " +
                        "AND id_revisionNumber=%d " +
                        "AND id_statusId_toolchain='%s' "
        ).formatted(
                testId.getTestId(), testId.getSuiteId(), testId.getBranch(), revision, testId.getToolchain()
        );

        var query = "SELECT status, count(*) as number\n" +
                "FROM " + table + "\n" +
                "WHERE " + filter + "\n" +
                "GROUP BY status";

        return "%s\n%s".formatted(declarations, query);
    }

    @Override
    public QueryType getQueryType() {
        return QueryType.SELECT;
    }

    @Override
    public String toDebugString(Object params) {
        return this.getClass().getSimpleName();
    }

    @Value
    public static class Result implements yandex.cloud.repository.db.Table.View {
        Common.TestStatus status;

        @Column(dbType = DbType.UINT64)
        Long number;
    }
}
