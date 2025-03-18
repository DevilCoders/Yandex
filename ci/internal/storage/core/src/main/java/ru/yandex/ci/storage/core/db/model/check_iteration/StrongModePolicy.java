package ru.yandex.ci.storage.core.db.model.check_iteration;

import ru.yandex.ci.ydb.Persisted;

@Persisted
public enum StrongModePolicy {
    BY_A_YAML,
    FORCE_ON,
    FORCE_OFF,
}
