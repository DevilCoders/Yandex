package ru.yandex.ci.core.security;

import javax.annotation.Nonnull;

import lombok.Value;

import ru.yandex.ci.core.arc.OrderedArcRevision;

@Value(staticConstructor = "of")
public class OwnerConfig {
    @Nonnull
    OrderedArcRevision revision; // Launch revision

    @Nonnull
    String owner;
}
