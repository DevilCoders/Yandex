package ru.yandex.ci.flow.engine.source_code;

import java.util.Optional;
import java.util.UUID;
import java.util.function.Function;

import com.google.common.base.Preconditions;

import ru.yandex.ci.flow.engine.definition.job.JobExecutor;
import ru.yandex.ci.flow.engine.runtime.state.model.ExecutorContext;
import ru.yandex.ci.flow.engine.source_code.model.JobExecutorObject;
import ru.yandex.ci.flow.engine.source_code.model.SourceCodeObject;
import ru.yandex.ci.flow.engine.source_code.model.SourceCodeObjectType;

public interface SourceCodeService {
    /**
     * Try get source code entity class by UUID
     *
     * @param uuid id to lookup
     * @return source code
     */
    Optional<SourceCodeObject<?>> lookup(UUID uuid);

    /**
     * Check if executor is valid
     *
     * @param executorContext context
     */
    void validateJobExecutor(ExecutorContext executorContext);

    /**
     * Try get job executor
     *
     * @param executorContext context
     * @return executor object
     */
    Optional<JobExecutorObject> lookupJobExecutor(ExecutorContext executorContext);

    /**
     * Get job executor
     *
     * @param executorContext context
     * @return executor object
     */
    JobExecutorObject getJobExecutor(ExecutorContext executorContext);

    /**
     * Get job executor bean
     *
     * @param jobExecutorObject job executor object
     * @return object or throw exception if we can't resolve a bean with exact type
     */
    JobExecutor resolveJobExecutorBean(JobExecutorObject jobExecutorObject);

    default Function<SourceCodeObject<?>, SourceCodeObject<?>> type(SourceCodeObjectType type) {
        return object -> {
            Preconditions.checkState(object.getType() == type,
                    "Object with UUID %s has incorrect object type, expect %s, got %s",
                    object.getId(), type, object.getType());
            return object;
        };
    }

    default <T extends SourceCodeObject<?>> Function<SourceCodeObject<?>, T> cast(Class<T> clazz) {
        return object -> {
            Preconditions.checkState(clazz.isAssignableFrom(object.getClass()),
                    "Object with UUID %s has incorrect class, expect %s, got %s",
                    object.getId(), clazz, object.getClass());
            return clazz.cast(object);
        };
    }
}
