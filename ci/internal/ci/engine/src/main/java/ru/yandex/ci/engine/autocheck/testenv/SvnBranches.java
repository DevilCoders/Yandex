package ru.yandex.ci.engine.autocheck.testenv;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

// Class is a copy of https://nda.ya.ru/t/pYV3ZGoo4Gc4q8
final class SvnBranches {
    private static final String TRUNK = "trunk";
    private static final String PREFIX = "branches/";

    private SvnBranches() {
    }

    @Nullable
    static String branchWithoutPrefixOrNull(@Nonnull String branch) {
        if (TRUNK.equals(branch)) {
            return null;
        }

        if (branch.startsWith(PREFIX)) {
            return branch.substring(PREFIX.length());
        }

        return branch;
    }
}
