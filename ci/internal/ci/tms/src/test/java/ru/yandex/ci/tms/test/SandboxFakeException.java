package ru.yandex.ci.tms.test;

/**
 * Created by azee on 02.12.16.
 */
public class SandboxFakeException extends RuntimeException {

    public SandboxFakeException(String message) {
        super(message);
    }

    public SandboxFakeException(String message, Throwable cause) {
        super(message, cause);
    }
}
