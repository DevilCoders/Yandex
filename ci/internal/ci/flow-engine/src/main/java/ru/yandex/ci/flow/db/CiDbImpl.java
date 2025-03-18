package ru.yandex.ci.flow.db;

import lombok.experimental.Delegate;

import yandex.cloud.repository.db.Tx;
import yandex.cloud.repository.db.TxManager;
import yandex.cloud.repository.db.TxManagerImpl;
import yandex.cloud.repository.db.YqlVersion;

import ru.yandex.ci.common.temporal.ydb.TemporalTables;
import ru.yandex.ci.core.db.CiMainTables;
import ru.yandex.ci.util.CiJson;
import ru.yandex.commune.bazinga.ydb.storage.BazingaStorageDbTables;

@SuppressWarnings("MissingOverride")
public class CiDbImpl implements CiDb {

    static {
        CiJson.init();
    }

    @Delegate
    private final TxManager txManager;

    public CiDbImpl(CiDbRepository ciDbRepository) {
        this.txManager = new TxManagerImpl(ciDbRepository).withYqlVersion(YqlVersion.YQLv1);
    }

    @Delegate
    private CiMainTables mainTables() {
        return (CiMainTables) Tx.Current.get().getRepositoryTransaction();
    }

    @Delegate
    private CiFlowTables flowTables() {
        return (CiFlowTables) Tx.Current.get().getRepositoryTransaction();
    }

    @Delegate
    private TemporalTables temporalTables() {
        return (TemporalTables) Tx.Current.get().getRepositoryTransaction();
    }

    @Delegate
    private BazingaStorageDbTables bazingaTables() {
        return (BazingaStorageDbTables) Tx.Current.get().getRepositoryTransaction();
    }
}
