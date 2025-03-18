package ru.yandex.ci.core.arc;

import java.nio.file.Path;
import java.util.Optional;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.Value;

@Value
public class ArcCommitWithPath {
    @Nonnull
    ArcCommit arcCommit;

    @Nullable
    Path path;

    public Optional<Path> getPath() {
        return Optional.ofNullable(path);
    }
}
