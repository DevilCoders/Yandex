package ru.yandex.ci.common.temporal.ydb;

import yandex.cloud.repository.db.TxManager;

public interface TemporalDb extends TxManager, TemporalTables {
}
