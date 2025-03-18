package ru.yandex.ci.core.config.a.model;

import java.util.List;

import javax.annotation.Nonnull;

import lombok.Value;

@Value
public class NativeBuildConfig {
    @Nonnull
    String toolchain;

    @Nonnull
    List<String> targets;
}
