package ru.yandex.ci.observer.api.stress_test;

import javax.annotation.Nonnull;

import lombok.Value;

@Value
public class UsedRevisionResponseDto {

    @Nonnull
    String rightCommitId;

    @Nonnull
    String namespace;

    long leftRevisionNumber;

    @Nonnull
    String flowLaunchId;

}
