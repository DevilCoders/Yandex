package ru.yandex.ci.flow.db;

import yandex.cloud.repository.db.TxManager;

import ru.yandex.ci.common.ydb.TransactionSupportDefault;

public interface CiFlowDb extends CiFlowTables, TxManager, TransactionSupportDefault {
}
