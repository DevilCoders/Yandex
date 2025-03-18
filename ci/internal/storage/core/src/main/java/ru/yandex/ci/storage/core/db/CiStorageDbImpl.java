package ru.yandex.ci.storage.core.db;

import lombok.experimental.Delegate;

import yandex.cloud.repository.db.Tx;
import yandex.cloud.repository.db.TxManager;
import yandex.cloud.repository.db.TxManagerImpl;
import yandex.cloud.repository.db.YqlVersion;

import ru.yandex.ci.common.temporal.ydb.TemporalTables;
import ru.yandex.commune.bazinga.ydb.storage.BazingaStorageDbTables;
import ru.yandex.lang.NonNullApi;

@SuppressWarnings("MissingOverride")
@NonNullApi
public class CiStorageDbImpl implements CiStorageDb {
    @Delegate
    private final TxManager txManager;

    public CiStorageDbImpl(CiStorageRepository repository) {
        this(repository, false);
    }

    public CiStorageDbImpl(CiStorageRepository repository, boolean skipSetup) {
        this.txManager = new TxManagerImpl(repository)
                .withYqlVersion(YqlVersion.YQLv1)
                .withMaxRetries(5); // Let's keep this number low, retries are not cheap, storage must avoid them.

        if (!skipSetup) {
            this.currentOrTx(() -> this.settings().setup());
        }
    }

    @Delegate(types = {CiStorageDbTables.class, BazingaStorageDbTables.class, TemporalTables.class})
    private CiStorageDbTables storageTables() {
        var transaction = Tx.Current.get();
        return (CiStorageDbTables) transaction.getRepositoryTransaction();
    }
}
