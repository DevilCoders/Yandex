package ru.yandex.ci.core.config.branch.validation;

import lombok.EqualsAndHashCode;
import lombok.Value;

@Value
@EqualsAndHashCode(callSuper = false)
public class BranchYamlValidationException extends Exception {
    BranchYamlValidationReport validationReport;

    @Override
    public String getMessage() {
        return validationReport.toString();
    }
}
