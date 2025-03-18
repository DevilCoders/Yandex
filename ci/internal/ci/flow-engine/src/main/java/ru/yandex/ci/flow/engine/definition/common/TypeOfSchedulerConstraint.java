package ru.yandex.ci.flow.engine.definition.common;

import ru.yandex.ci.ydb.Persisted;

/**
 * Тип ограничения для расписания запуксков {@link JobSchedulerConstraintEntity}.
 * WORK - ограничение на запуск в рабочие дни
 * PRE_HOLIDAY - ограничение на запуск в пред выходные дни
 * HOLIDAY - ограничение на запуск в выходные дни
 */
@Persisted
public enum TypeOfSchedulerConstraint {
    WORK,
    PRE_HOLIDAY,
    HOLIDAY
}
