package ru.yandex.ci.flow.db;

import ru.yandex.ci.common.temporal.ydb.TemporalDb;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.commune.bazinga.ydb.storage.BazingaStorageDb;

public interface CiDb extends CiFlowDb, CiMainDb, BazingaStorageDb, TemporalDb {
}
