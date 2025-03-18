package ru.yandex.ci.core.db;

import lombok.experimental.Delegate;

import yandex.cloud.repository.db.Tx;
import yandex.cloud.repository.db.TxManager;
import yandex.cloud.repository.db.TxManagerImpl;
import yandex.cloud.repository.db.YqlVersion;

import ru.yandex.ci.util.CiJson;

@SuppressWarnings("MissingOverride")
public class CiMainDbImpl implements CiMainDb {

    static {
        CiJson.init();
    }

    @Delegate
    private final TxManager txManager;

    public CiMainDbImpl(CiMainRepository ciMainRepository) {
        this.txManager = new TxManagerImpl(ciMainRepository).withYqlVersion(YqlVersion.YQLv1);
    }

    @Delegate
    private CiMainTables ciTables() {
        return (CiMainTables) Tx.Current.get().getRepositoryTransaction();
    }

}
