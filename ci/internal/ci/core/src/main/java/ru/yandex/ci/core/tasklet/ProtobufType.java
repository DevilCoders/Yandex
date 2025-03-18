package ru.yandex.ci.core.tasklet;

import lombok.Value;

@Value(staticConstructor = "of")
public class ProtobufType {
    String fullMessageType;
}
