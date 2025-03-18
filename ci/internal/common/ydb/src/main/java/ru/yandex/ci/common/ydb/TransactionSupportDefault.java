package ru.yandex.ci.common.ydb;

import java.util.function.Supplier;

import yandex.cloud.repository.db.IsolationLevel;
import yandex.cloud.repository.db.Tx;
import yandex.cloud.repository.db.TxManager;

/**
 * This is a temporary interface compatible with standard {@link TxManager}.
 * Should be removed after complete migration to {@link TransactionSupport}
 */
public interface TransactionSupportDefault extends TxManager, TransactionSupport {

    // Support working inside any transaction type
    @Override
    default <T> T currentOrReadOnly(Supplier<T> supplier) {
        return currentOrReadOnlyImpl(supplier);
    }

    @Override
    default void currentOrReadOnly(Runnable runnable) {
        currentOrReadOnlyImpl(() -> {
            runnable.run();
            return null;
        });
    }

    // Support only RW transaction
    @Override
    default <T> T currentOrTx(Supplier<T> supplier) {
        return currentOrTxImpl(supplier);
    }

    @Override
    default void currentOrTx(Runnable runnable) {
        currentOrTxImpl(() -> {
            runnable.run();
            return null;
        });
    }

    @Override
    default void tx(Runnable runnable) {
        this.withName(YdbUtils.location(1)).tx(() -> {
            runnable.run();
            return null;
        });
    }


    @Override
    default <T> T readOnly(Supplier<T> supplier) {
        return withName(YdbUtils.location(1)).readOnly().run(supplier);
    }

    @Override
    default void readOnly(Runnable runnable) {
        withName(YdbUtils.location(1)).readOnly().run(() -> {
            runnable.run();
            return null;
        });
    }

    //

    private <T> T currentOrReadOnlyImpl(Supplier<T> supplier) {
        if (Tx.Current.exists()) {
            return supplier.get();
        } else {
            return withName(YdbUtils.location(2)).readOnly().run(supplier);
        }
    }

    private <T> T currentOrTxImpl(Supplier<T> supplier) {
        if (Tx.Current.exists()) {
            var tx = Tx.Current.get().getRepositoryTransaction();
            if (tx instanceof KikimrRepositoryCi.KikimrRepositoryTransactionCi<?> txCi) {
                var isolationLevel = txCi.getOptions().getIsolationLevel();
                if (isolationLevel != IsolationLevel.SERIALIZABLE_READ_WRITE) {
                    throw new IllegalStateException(String.format(
                            "Unable to join into transaction, current isolation level is %s, must be %s",
                            isolationLevel, IsolationLevel.SERIALIZABLE_READ_WRITE));
                }
            }
            return supplier.get();
        } else {
            return this.withName(YdbUtils.location(2)).tx(supplier);
        }
    }
}
