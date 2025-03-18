package ru.yandex.ci.client.observer;

import lombok.Value;

@Value(staticConstructor = "of")
public class OrderedRevisionDto {
    String branch;
    String revision;
    long revisionNumber;
}
