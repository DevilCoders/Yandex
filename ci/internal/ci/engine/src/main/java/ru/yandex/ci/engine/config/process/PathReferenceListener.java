package ru.yandex.ci.engine.config.process;

import java.nio.file.Path;

import ru.yandex.ci.core.arc.ArcRevision;

public interface PathReferenceListener {
    void onConfig(Path path, ArcRevision revision) throws Exception;
}
