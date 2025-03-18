package ru.yandex.ci.observer.core.db;

import javax.annotation.Nonnull;

import yandex.cloud.repository.db.RepositoryTransaction;
import yandex.cloud.repository.db.TxOptions;
import yandex.cloud.repository.kikimr.KikimrConfig;
import yandex.cloud.repository.kikimr.KikimrRepository;
import ru.yandex.ci.common.ydb.KikimrRepositoryCi;
import ru.yandex.ci.observer.core.db.model.check.CheckTable;
import ru.yandex.ci.observer.core.db.model.check_iterations.CheckIterationsTable;
import ru.yandex.ci.observer.core.db.model.check_tasks.CheckTasksTable;
import ru.yandex.ci.observer.core.db.model.settings.ObserverSettingsTable;
import ru.yandex.ci.observer.core.db.model.sla_statistics.SlaStatisticsTable;
import ru.yandex.ci.observer.core.db.model.stress_test.StressTestUsedCommitTable;
import ru.yandex.ci.observer.core.db.model.traces.CheckTaskPartitionTracesTable;
import ru.yandex.lang.NonNullApi;

public class CiObserverRepository extends KikimrRepositoryCi {
    public CiObserverRepository(@Nonnull KikimrConfig config) {
        super(config);
    }

    @Override
    public RepositoryTransaction startTransaction(TxOptions options) {
        return new CiObserverDbTransaction<>(this, options);
    }

    @NonNullApi
    public static class CiObserverDbTransaction<R extends KikimrRepository>
            extends KikimrRepositoryTransactionCi<R> implements CiObserverDbTables {

        public CiObserverDbTransaction(R kikimrRepository, @Nonnull TxOptions options) {
            super(kikimrRepository, options);
        }

        @Override
        public CheckTable checks() {
            return new CheckTable(this);
        }

        @Override
        public CheckTasksTable tasks() {
            return new CheckTasksTable(this);
        }

        @Override
        public ObserverSettingsTable settings() {
            return new ObserverSettingsTable(this);
        }

        @Override
        public CheckIterationsTable iterations() {
            return new CheckIterationsTable(this);
        }

        @Override
        public CheckTaskPartitionTracesTable traces() {
            return new CheckTaskPartitionTracesTable(this);
        }

        @Override
        public SlaStatisticsTable slaStatistics() {
            return new SlaStatisticsTable(this);
        }

        @Override
        public StressTestUsedCommitTable stressTestUsedCommitTable() {
            return new StressTestUsedCommitTable(this);
        }
    }
}
