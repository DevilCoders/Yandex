package ru.yandex.ci.storage.post_processor.processing;

import lombok.Value;

import ru.yandex.ci.logbroker.MessagesCountdown;

@Value
public class ResultMessage {
    MessagesCountdown countdown;
    PostProcessorTestResult result;
}
