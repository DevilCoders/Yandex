package ru.yandex.ci.core.config.branch;

import java.nio.file.Path;

import lombok.Value;

import ru.yandex.ci.core.config.a.ConfigChangeType;

@Value
public class AffectedBranchYaml {
    Path path;
    ConfigChangeType changeType;
}
