package ru.yandex.ci.ayamler;

import java.nio.file.Path;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.Value;

@Value(staticConstructor = "of")
public class PathAndLogin {
    @Nonnull
    Path path;
    @Nullable
    String login;   // null until storage starts to send login
}
