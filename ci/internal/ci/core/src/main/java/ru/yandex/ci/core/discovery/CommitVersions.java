package ru.yandex.ci.core.discovery;

import lombok.Value;

@Value
public class CommitVersions {
    String version;
    boolean branch;
    boolean release;
}
