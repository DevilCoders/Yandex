package ru.yandex.ci.storage.core.cache;

import java.util.function.Consumer;
import java.util.function.Function;

import ru.yandex.ci.common.ydb.TransactionSupportDefault;

public interface StorageCache<Modifiable, CiDb extends TransactionSupportDefault> {

    CiDb getDb();

    void modify(Consumer<Modifiable> callback);

    <T> T modifyAndGet(Function<Modifiable, T> callback);

    void modifyWithDbTx(Consumer<Modifiable> callback);

    void modifyWithDbReadonly(Consumer<Modifiable> callback);

    <T> T modifyWithDbTxAndGet(Function<Modifiable, T> callback);
}
