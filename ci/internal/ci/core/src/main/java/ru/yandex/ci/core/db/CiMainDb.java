package ru.yandex.ci.core.db;

import yandex.cloud.repository.db.TxManager;

import ru.yandex.ci.common.ydb.TransactionSupportDefault;
import ru.yandex.lang.NonNullApi;

@NonNullApi
public interface CiMainDb extends CiMainTables, TxManager, TransactionSupportDefault {
}
