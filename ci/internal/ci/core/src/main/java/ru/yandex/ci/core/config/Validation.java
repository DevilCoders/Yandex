package ru.yandex.ci.core.config;

import javax.annotation.Nullable;

import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.core.arc.ArcCommit;

@Slf4j
public class Validation {
    private Validation() {
    }

    public static boolean applySince(@Nullable ArcCommit commit, int svnRevision) {
        if (commit == null || !commit.isTrunk() || commit.getSvnRevision() > svnRevision) {
            return true;
        }

        log.info("Validation skipped at {}", commit);
        return false;
    }
}
