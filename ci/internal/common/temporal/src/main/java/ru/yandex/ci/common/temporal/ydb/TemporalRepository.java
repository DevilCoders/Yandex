package ru.yandex.ci.common.temporal.ydb;

import lombok.NonNull;

import yandex.cloud.repository.db.RepositoryTransaction;
import yandex.cloud.repository.db.TxOptions;
import yandex.cloud.repository.kikimr.KikimrConfig;
import yandex.cloud.repository.kikimr.KikimrRepository;
import yandex.cloud.repository.kikimr.KikimrRepositoryTransaction;
import yandex.cloud.repository.kikimr.table.KikimrTable;
import ru.yandex.ci.common.temporal.monitoring.TemporalFailingWorkflowTable;

public class TemporalRepository extends KikimrRepository {

    public TemporalRepository(@NonNull KikimrConfig config) {
        super(config);
    }

    @Override
    public RepositoryTransaction startTransaction(TxOptions options) {
        return new DbTransaction<>(this, options);
    }

    public static class DbTransaction<R extends KikimrRepository>
            extends KikimrRepositoryTransaction<R> implements TemporalTables {

        public DbTransaction(R kikimrRepository, @NonNull TxOptions options) {
            super(kikimrRepository, options);
        }

        @Override
        public TemporalLaunchQueueTable temporalLaunchQueue() {
            return new TemporalLaunchQueueTable(this);
        }

        @Override
        public TemporalFailingWorkflowTable temporalFailingWorkflow() {
            return new TemporalFailingWorkflowTable(this);
        }
    }

    public static class DbTransactionProxy implements TemporalTables {
        private final KikimrTable.QueryExecutor executor;

        public DbTransactionProxy(KikimrTable.QueryExecutor executor) {
            this.executor = executor;
        }

        @Override
        public TemporalLaunchQueueTable temporalLaunchQueue() {
            return new TemporalLaunchQueueTable(executor);
        }

        @Override
        public TemporalFailingWorkflowTable temporalFailingWorkflow() {
            return new TemporalFailingWorkflowTable(executor);
        }
    }
}
