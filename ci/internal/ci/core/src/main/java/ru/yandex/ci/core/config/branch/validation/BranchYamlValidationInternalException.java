package ru.yandex.ci.core.config.branch.validation;

public class BranchYamlValidationInternalException extends RuntimeException {
    public BranchYamlValidationInternalException(String message) {
        super(message);
    }

    public BranchYamlValidationInternalException(Throwable cause) {
        super(cause);
    }
}
