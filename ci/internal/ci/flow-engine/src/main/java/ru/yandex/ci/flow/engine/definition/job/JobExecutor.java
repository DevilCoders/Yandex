package ru.yandex.ci.flow.engine.definition.job;

import ru.yandex.ci.core.common.SourceCodeEntity;
import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.resources.Consume;
import ru.yandex.ci.flow.engine.definition.resources.Produces;

/**
 * Основная единица работы в флоу.
 * Может производить результат, в этом случае нужно повесить аннотацию {@link Produces} на класс.
 * Может запросить результаты других джоб, @see {@link Consume}
 */
public interface JobExecutor extends SourceCodeEntity {
    /**
     * Запуск задачи.
     *
     * @param context контекст задачи
     */
    void execute(JobContext context) throws Exception;

    /**
     * Прерывает выполнение задачи, завершая внешние задачи.
     *
     * @param context контекст задачи
     */
    default void interrupt(JobContext context) {
    }
}
