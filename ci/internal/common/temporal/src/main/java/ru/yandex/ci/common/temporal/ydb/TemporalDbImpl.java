package ru.yandex.ci.common.temporal.ydb;

import lombok.experimental.Delegate;

import yandex.cloud.repository.db.Tx;
import yandex.cloud.repository.db.TxManager;
import yandex.cloud.repository.db.TxManagerImpl;
import yandex.cloud.repository.db.YqlVersion;

@SuppressWarnings("MissingOverride")
public class TemporalDbImpl implements TemporalDb {
    private final TxManager txManager;

    public TemporalDbImpl(TemporalRepository repository) {
        this.txManager = new TxManagerImpl(repository).withYqlVersion(YqlVersion.YQLv1);
    }

    @Delegate
    private TemporalTables bazingaDbTables() {
        return (TemporalTables) Tx.Current.get().getRepositoryTransaction();
    }

    @Delegate
    private TxManager getTxManager() {
        return txManager;
    }
}
