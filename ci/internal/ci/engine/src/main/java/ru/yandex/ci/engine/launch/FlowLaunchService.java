package ru.yandex.ci.engine.launch;

import ru.yandex.ci.core.config.a.validation.AYamlValidationException;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.engine.config.ConfigBundle;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.state.model.JobType;

/**
 * Инициирует запуск конкретного flow.
 */
public interface FlowLaunchService {
    /**
     * Запускает flow конкретного процесса. В случае ошибки бросает исключение.
     * Перезапуск организуется снаружи, передавайте LaunchId с инкрементированным number.
     */
    FlowLaunchId launchFlow(Launch launch, ConfigBundle bundle) throws AYamlValidationException;

    /**
     * Запускает остановку flow
     */
    void cancelFlow(FlowLaunchId flowLaunchId);

    /**
     * Запускает остановку задач определенного типа
     *
     * @param flowLaunchId flow launch
     * @param jobType      тип задачи
     */
    void cancelJobs(FlowLaunchId flowLaunchId, JobType jobType);

    /**
     * Запускает очистку flow
     */
    void cleanupFlow(FlowLaunchId flowLaunchId);
}
