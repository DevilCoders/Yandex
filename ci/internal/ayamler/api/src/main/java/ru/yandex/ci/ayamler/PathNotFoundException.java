package ru.yandex.ci.ayamler;

import java.nio.file.Path;

import javax.annotation.Nonnull;

import ru.yandex.ci.core.arc.ArcRevision;

public class PathNotFoundException extends RuntimeException {

    @Nonnull
    private final ArcRevision revision;
    @Nonnull
    private final Path path;

    public PathNotFoundException(ArcRevision revision, Path path) {
        super("not found %s at %s".formatted(path, revision));
        this.revision = revision;
        this.path = path;
    }

    public ArcRevision getRevision() {
        return revision;
    }

    public Path getPath() {
        return path;
    }
}
