package ru.yandex.ci.core.registry;

import java.util.List;

import yandex.cloud.repository.db.EntitySchema;
import yandex.cloud.repository.db.YqlVersion;
import yandex.cloud.repository.kikimr.statement.YqlStatement;
import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;

import ru.yandex.ci.common.ydb.KikimrTableCi;
public class RegistryTaskTable extends KikimrTableCi<RegistryTask> {

    public RegistryTaskTable(QueryExecutor executor) {
        super(RegistryTask.class, executor);
    }

    public void markStaleAll() {
        executor.execute(new MarkStaleAll(), null);
    }

    public List<RegistryTask> findAlive() {
        return find(YqlPredicate.where("stale").eq(false));
    }

    private static class MarkStaleAll extends YqlStatement<Void, RegistryTask, RegistryTask> {
        private MarkStaleAll() {
            super(EntitySchema.of(RegistryTask.class), EntitySchema.of(RegistryTask.class));
        }

        @Override
        public String getQuery(String tablespace, YqlVersion yqlVersion) {
            return "UPDATE " + this.table(tablespace) + " SET stale = true";
        }

        @Override
        public String toDebugString(Void unused) {
            return "markStaleAll";
        }

        @Override
        public QueryType getQueryType() {
            return QueryType.UPDATE;
        }
    }
}
