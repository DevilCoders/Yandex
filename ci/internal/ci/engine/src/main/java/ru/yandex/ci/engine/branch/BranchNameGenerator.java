package ru.yandex.ci.engine.branch;

import ru.yandex.ci.core.config.a.model.ReleaseConfig;

public interface BranchNameGenerator {
    String generateName(ReleaseConfig releaseConfig, String version, int seq);
}
