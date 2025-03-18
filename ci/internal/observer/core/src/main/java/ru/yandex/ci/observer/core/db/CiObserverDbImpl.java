package ru.yandex.ci.observer.core.db;

import lombok.experimental.Delegate;

import yandex.cloud.repository.db.Tx;
import yandex.cloud.repository.db.TxManager;
import yandex.cloud.repository.db.TxManagerImpl;
import yandex.cloud.repository.db.YqlVersion;

import ru.yandex.lang.NonNullApi;

@SuppressWarnings("MissingOverride")
@NonNullApi
public class CiObserverDbImpl implements CiObserverDb {
    @Delegate
    private final TxManager txManager;

    public CiObserverDbImpl(CiObserverRepository repository) {
        this.txManager = new TxManagerImpl(repository)
                .withYqlVersion(YqlVersion.YQLv1)
                .withMaxRetries(20);
    }

    @Delegate(types = {CiObserverDbTables.class})
    private CiObserverDbTables observerTables() {
        var transaction = Tx.Current.get();
        return (CiObserverDbTables) transaction.getRepositoryTransaction();
    }
}
