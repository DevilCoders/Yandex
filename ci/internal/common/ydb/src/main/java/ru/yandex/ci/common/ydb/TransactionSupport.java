package ru.yandex.ci.common.ydb;

import java.util.function.Supplier;

/**
 * Interface for YDB transaction support:
 * - hides actual implementation details of YDB ORM transactional manager
 * - simplify API usage
 */
public interface TransactionSupport {

    // Recommended methods for working within transactions

    <T> T currentOrReadOnly(Supplier<T> supplier);

    void currentOrReadOnly(Runnable runnable);

    <T> T currentOrTx(Supplier<T> supplier);

    void currentOrTx(Runnable runnable);


    // Explicitly starts new transaction

    <T> T tx(Supplier<T> supplier);

    void tx(Runnable runnable);

    <T> T readOnly(Supplier<T> supplier);

    void readOnly(Runnable runnable);

}
