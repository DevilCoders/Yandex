package ru.yandex.ci.client.sandbox.api;

/**
 * @author albazh
 */
public enum SandboxTaskType {
    JAVA_TESTPALM_TASK("JAVA_TESTPALM_TASK");

    private final String code;

    SandboxTaskType(String code) {
        this.code = code;
    }

    public String getCode() {
        return code;
    }
}
