package ru.yandex.ci.client.ayamler;

import javax.annotation.Nonnull;

import lombok.Value;

@Value
public class StrongModeRequest {

    @Nonnull
    String path;

    @Nonnull
    String revision;

    @Nonnull
    String login;
}
