package ru.yandex.ci.engine.launch;

public class ProjectNotInWhiteListException extends RuntimeException {
    public ProjectNotInWhiteListException() {
        super("Project is not in white list");
    }
}
