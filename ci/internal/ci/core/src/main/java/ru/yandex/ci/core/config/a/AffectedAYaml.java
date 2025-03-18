package ru.yandex.ci.core.config.a;

import java.nio.file.Path;

public class AffectedAYaml {
    private final Path path;
    private final ConfigChangeType changeType;

    public AffectedAYaml(Path path, ConfigChangeType changeType) {
        this.path = path;
        this.changeType = changeType;
    }

    public Path getPath() {
        return path;
    }

    public ConfigChangeType getChangeType() {
        return changeType;
    }

    @Override
    public String toString() {
        return "AffectedAYaml{" +
                "path=" + path +
                ", changeType=" + changeType +
                '}';
    }
}
