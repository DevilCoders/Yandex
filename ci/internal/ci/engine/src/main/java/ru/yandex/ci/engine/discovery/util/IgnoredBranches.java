package ru.yandex.ci.engine.discovery.util;

import java.util.Set;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import ru.yandex.ci.core.arc.ArcBranch;

// https://st.yandex-team.ru/CI-1567
public class IgnoredBranches {
    private static final Logger log = LoggerFactory.getLogger(IgnoredBranches.class);
    private static final Set<String> IGNORED_PREFIXES = Set.of(
            "releases/experimental/mobile/",
            "releases/experimental/navi/",
            "releases/geoadv/backend/testing/yml-feed"
    );

    private IgnoredBranches() {
    }

    public static boolean isIgnored(ArcBranch branch) {
        boolean ignored = IGNORED_PREFIXES.stream().anyMatch(branch.asString()::startsWith);
        if (ignored) {
            log.info("Branch {} is ignored", branch);
        }
        return ignored;
    }

}
