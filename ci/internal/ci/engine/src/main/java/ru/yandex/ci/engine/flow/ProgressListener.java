package ru.yandex.ci.engine.flow;

public interface ProgressListener {
    ProgressListener NO_OP = taskProgressEvent -> {
    };

    void updated(TaskProgressEvent taskProgressEvent);
}
