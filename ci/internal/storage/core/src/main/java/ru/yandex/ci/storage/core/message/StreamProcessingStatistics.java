package ru.yandex.ci.storage.core.message;

public interface StreamProcessingStatistics {
    void onMessageProcessed(Enum<?> messageCase, int value);

    void onParseError();

    void onValidationError();

    void onMissingError();

    void onFinishedStateError();
}
