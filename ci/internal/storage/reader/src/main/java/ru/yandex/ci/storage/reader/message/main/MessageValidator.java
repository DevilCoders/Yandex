package ru.yandex.ci.storage.reader.message.main;

import ru.yandex.ci.storage.core.CheckTaskOuterClass;
import ru.yandex.ci.storage.reader.exceptions.ReaderValidationException;

public class MessageValidator {
    private MessageValidator() {

    }

    public static void validate(CheckTaskOuterClass.FullTaskId id) {
        checkNotEmpty(id, "FullTaskId missing");
        checkNotEmpty(id.getIterationId(), "IterationId missing");
        checkNotEmpty(id.getTaskId(), "TaskId missing");
    }

    private static void checkNotEmpty(Object object, String message) {
        checkState(object != null, message);
    }

    private static void checkNotEmpty(String object, String message) {
        checkState(object != null && !object.isEmpty(), message);
    }

    private static void checkState(boolean condition, String message) {
        if (!condition) {
            throw new ReaderValidationException(message);
        }
    }
}
