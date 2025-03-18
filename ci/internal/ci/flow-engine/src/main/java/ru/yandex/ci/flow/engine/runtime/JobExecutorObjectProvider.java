package ru.yandex.ci.flow.engine.runtime;

import ru.yandex.ci.flow.engine.runtime.state.model.ExecutorContext;
import ru.yandex.ci.flow.engine.source_code.model.JobExecutorObject;

public interface JobExecutorObjectProvider {
    JobExecutorObject createExecutorObject(ExecutorContext executorContext);
}
