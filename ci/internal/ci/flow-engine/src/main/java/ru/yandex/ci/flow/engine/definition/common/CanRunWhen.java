package ru.yandex.ci.flow.engine.definition.common;

import ru.yandex.ci.ydb.Persisted;

@Persisted
public enum CanRunWhen {
    /*
     * Когда ровно N из N апстримов готово, N >= 0
     */
    ALL_COMPLETED,
    /*
     * Когда 1 <= k <= N апстримов готово из N, N >= 0
     */
    ANY_COMPLETED,

    /*
     * Когда есть хотя бы один из апстримов, который завершился ошибкой
     */
    ANY_FAILED;
}
