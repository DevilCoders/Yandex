package ru.yandex.ci.core.config.registry;

import lombok.Getter;

public class TaskRegistryValidationException extends TaskRegistryException {
    @Getter
    private final TaskRegistryValidationReport validationReport;

    public TaskRegistryValidationException(TaskRegistryValidationReport validationReport) {
        super(validationReport.toString());
        this.validationReport = validationReport;
    }
}
