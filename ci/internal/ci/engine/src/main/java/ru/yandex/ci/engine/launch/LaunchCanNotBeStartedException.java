package ru.yandex.ci.engine.launch;

public class LaunchCanNotBeStartedException extends RuntimeException {
    public LaunchCanNotBeStartedException(String message) {
        super(message);
    }
}
