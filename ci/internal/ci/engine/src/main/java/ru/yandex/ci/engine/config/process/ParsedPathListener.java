package ru.yandex.ci.engine.config.process;

import java.nio.file.Path;

import ru.yandex.ci.engine.config.ConfigParseResult;

public interface ParsedPathListener {
    void onParsedConfig(Path path, ConfigParseResult result) throws Exception;
}
