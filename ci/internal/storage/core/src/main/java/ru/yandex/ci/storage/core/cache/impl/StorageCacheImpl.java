package ru.yandex.ci.storage.core.cache.impl;

import java.util.function.Consumer;
import java.util.function.Function;

import com.google.common.base.Preconditions;

import yandex.cloud.repository.db.Tx;
import ru.yandex.ci.common.ydb.TransactionSupportDefault;
import ru.yandex.ci.storage.core.cache.EntityCache;
import ru.yandex.ci.storage.core.cache.StorageCache;
import ru.yandex.ci.util.ObjectStore;

public abstract class StorageCacheImpl<
        Modifiable extends EntityCache.Modifiable.CommitSupport,
        ModifiableInner extends Modifiable,
        CiDb extends TransactionSupportDefault
        > implements StorageCache<Modifiable, CiDb> {
    protected final CiDb db;

    @SuppressWarnings("ThreadLocalUsage")
    protected ThreadLocal<ModifiableInner> currentModifiable = new ThreadLocal<>();

    protected StorageCacheImpl(CiDb db) {
        this.db = db;
    }

    @Override
    public CiDb getDb() {
        return db;
    }

    @Override
    public void modify(Consumer<Modifiable> callback) {
        Preconditions.checkState(hasNoActiveTransaction(), "Transaction already opened");
        Preconditions.checkState(!Tx.Current.exists(), "Can not modify within transaction!");

        var cache = startTransaction();
        try {
            callback.accept(cache);
            commitTransaction();
        } catch (Throwable e) {
            rollbackTransaction();
            throw e;
        }
    }

    @Override
    public <T> T modifyAndGet(Function<Modifiable, T> callback) {
        Preconditions.checkState(hasNoActiveTransaction(), "Transaction already opened");
        Preconditions.checkState(!Tx.Current.exists(), "Can not modify within transaction!");

        var cache = startTransaction();
        try {
            var result = callback.apply(cache);
            commitTransaction();
            return result;
        } catch (Throwable e) {
            rollbackTransaction();
            throw e;
        }
    }

    @Override
    public void modifyWithDbTx(Consumer<Modifiable> callback) {
        Preconditions.checkState(hasNoActiveTransaction(), "Transaction already opened");
        Preconditions.checkState(!Tx.Current.exists(), "Can not modify within transaction!");

        try {
            this.db.currentOrTx(() -> callback.accept(startTransaction()));
            commitTransaction();
        } catch (Throwable e) {
            rollbackTransaction();
            throw e;
        }
    }

    @Override
    public void modifyWithDbReadonly(Consumer<Modifiable> callback) {
        Preconditions.checkState(hasNoActiveTransaction(), "Transaction already opened");
        Preconditions.checkState(!Tx.Current.exists(), "Can not modify within transaction!");

        try {
            this.db.readOnly().run(() -> callback.accept(startTransaction()));
            commitTransaction();
        } catch (Throwable e) {
            rollbackTransaction();
            throw e;
        }
    }

    @Override
    public <T> T modifyWithDbTxAndGet(Function<Modifiable, T> callback) {
        Preconditions.checkState(hasNoActiveTransaction(), "Transaction already opened");
        Preconditions.checkState(!Tx.Current.exists(), "Can not modify within transaction!");

        // in case of orm retries we recreate it every try
        var cache = new ObjectStore<ModifiableInner>();
        try {
            var result = this.db.currentOrTx(() -> {
                try {
                    return callback.apply(cache.set(startTransaction()));
                } catch (Throwable e) {
                    rollbackTransaction();
                    throw e;
                }
            });
            commitTransaction();
            return result;
        } catch (Throwable e) {
            rollbackTransaction();
            throw e;
        }
    }

    protected ModifiableInner startTransaction() {
        var modifiable = toModifiable();
        this.currentModifiable.set(modifiable);
        return modifiable;
    }

    protected void rollbackTransaction() {
        this.currentModifiable.remove();
    }

    protected void commitTransaction() {
        commit(this.currentModifiable.get());
        this.currentModifiable.remove();
    }

    protected boolean hasNoActiveTransaction() {
        return this.currentModifiable.get() == null;
    }

    protected abstract ModifiableInner toModifiable();

    protected abstract void commit(ModifiableInner cache);
}
