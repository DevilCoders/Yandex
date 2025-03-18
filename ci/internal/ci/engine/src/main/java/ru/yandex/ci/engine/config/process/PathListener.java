package ru.yandex.ci.engine.config.process;

import java.nio.file.Path;

public interface PathListener {
    void onConfig(Path path) throws Exception;
}
