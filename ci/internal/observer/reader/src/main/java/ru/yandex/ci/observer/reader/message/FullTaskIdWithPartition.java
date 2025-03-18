package ru.yandex.ci.observer.reader.message;

import lombok.Value;

import ru.yandex.ci.storage.core.CheckTaskOuterClass;

@Value
public class FullTaskIdWithPartition {
    CheckTaskOuterClass.FullTaskId fullTaskId;
    int partition;
}
