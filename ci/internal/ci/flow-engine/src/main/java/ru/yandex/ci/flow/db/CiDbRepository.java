package ru.yandex.ci.flow.db;

import javax.annotation.Nonnull;

import lombok.experimental.Delegate;

import yandex.cloud.repository.db.RepositoryTransaction;
import yandex.cloud.repository.db.TxOptions;
import yandex.cloud.repository.kikimr.KikimrConfig;
import yandex.cloud.repository.kikimr.KikimrRepository;

import ru.yandex.ci.common.temporal.ydb.TemporalRepository;
import ru.yandex.ci.common.temporal.ydb.TemporalTables;
import ru.yandex.ci.core.db.CiMainRepository;
import ru.yandex.ci.core.job.JobInstanceTable;
import ru.yandex.ci.flow.engine.runtime.di.ResourceTable;
import ru.yandex.ci.flow.engine.runtime.state.FlowLaunchTable;
import ru.yandex.ci.flow.engine.runtime.state.StageGroupTable;
import ru.yandex.commune.bazinga.ydb.storage.BazingaStorageDbTables;
import ru.yandex.commune.bazinga.ydb.storage.BazingaStorageRepository;
import ru.yandex.lang.NonNullApi;

public class CiDbRepository extends CiMainRepository {

    public CiDbRepository(@Nonnull KikimrConfig config) {
        super(config);
    }

    @Override
    public RepositoryTransaction startTransaction(TxOptions options) {
        return new CiDbRepositoryTransaction<>(this, options);
    }

    @SuppressWarnings("MissingOverride")
    @NonNullApi
    static class CiDbRepositoryTransaction<R extends KikimrRepository>
            extends CiMainRepository.CiMainRepositoryTransaction<R>
            implements CiFlowTables, BazingaStorageDbTables, TemporalTables {

        private final BazingaStorageRepository.BazingaStorageDbTablesProxy bazingaProxy;
        private final TemporalRepository.DbTransactionProxy temporalProxy;

        CiDbRepositoryTransaction(R kikimrRepository, @Nonnull TxOptions options) {
            super(kikimrRepository, options);
            this.bazingaProxy = new BazingaStorageRepository.BazingaStorageDbTablesProxy(this);
            this.temporalProxy = new TemporalRepository.DbTransactionProxy(this);
        }

        // flow

        @Override
        public JobInstanceTable jobInstance() {
            return new JobInstanceTable(this);
        }

        @Override
        public ResourceTable resources() {
            return new ResourceTable(this);
        }

        @Override
        public StageGroupTable stageGroup() {
            return new StageGroupTable(this);
        }

        @Override
        public FlowLaunchTable flowLaunch() {
            return new FlowLaunchTable(this);
        }


        @Delegate()
        public BazingaStorageDbTables bazingaProxy() {
            return this.bazingaProxy;
        }

        @Delegate()
        public TemporalTables temporalProxy() {
            return this.temporalProxy;
        }

    }
}
