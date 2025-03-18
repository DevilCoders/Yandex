package ru.yandex.ci.storage.core.logbroker;

import lombok.Value;

import ru.yandex.ci.storage.core.CheckOuterClass.CheckType;

@Value
public class LogbrokerTopic {
    CheckType type;
    String path;
    int numberOfPartitions;
}
