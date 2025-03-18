package ru.yandex.ci.observer.api.stress_test;

import lombok.Value;

@Value(staticConstructor = "of")
public class OrderedRevisionDto {
    String branch;
    String revision;
    long revisionNumber;
}
