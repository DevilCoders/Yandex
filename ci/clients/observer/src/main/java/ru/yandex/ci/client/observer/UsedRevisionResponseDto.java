package ru.yandex.ci.client.observer;

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
