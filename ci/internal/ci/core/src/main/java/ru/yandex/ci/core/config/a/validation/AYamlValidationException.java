package ru.yandex.ci.core.config.a.validation;

import java.util.Map;
import java.util.Objects;
import java.util.Set;

import ru.yandex.ci.core.config.a.validation.ValidationReport.FlowReport;

public class AYamlValidationException extends Exception {
    private final ValidationReport validationReport;

    public AYamlValidationException(ValidationReport validationReport) {
        this.validationReport = validationReport;
    }

    public AYamlValidationException(String jobId, String... errors) {
        this(ValidationReport.builder()
                .flowReport(new FlowReport(Map.of(jobId, Set.of(errors))))
                .build());
    }

    public ValidationReport getValidationReport() {
        return validationReport;
    }

    @Override
    public String getMessage() {
        return getValidationReport().toString();
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) {
            return true;
        }
        if (!(o instanceof AYamlValidationException)) {
            return false;
        }
        AYamlValidationException that = (AYamlValidationException) o;
        return Objects.equals(validationReport, that.validationReport);
    }

    @Override
    public int hashCode() {
        return Objects.hash(validationReport);
    }

    @Override
    public String toString() {
        return "AYamlValidationException{" +
                "validationReport=" + validationReport +
                '}';
    }
}
