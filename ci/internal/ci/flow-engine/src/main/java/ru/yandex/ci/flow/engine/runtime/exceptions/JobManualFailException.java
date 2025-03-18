package ru.yandex.ci.flow.engine.runtime.exceptions;

import java.util.List;

import ru.yandex.ci.flow.engine.definition.context.impl.SupportType;

public class JobManualFailException extends RuntimeException {
    private final List<SupportType> supportInfo;

    public JobManualFailException(String message, List<SupportType> supportInfo) {
        super(message);
        this.supportInfo = supportInfo;
    }

    public JobManualFailException(String message, List<SupportType> supportInfo, Throwable cause) {
        super(message, cause);
        this.supportInfo = supportInfo;
    }

    public List<SupportType> getSupportInfo() {
        return supportInfo;
    }
}
