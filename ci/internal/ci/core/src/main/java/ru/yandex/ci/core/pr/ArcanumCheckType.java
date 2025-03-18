package ru.yandex.ci.core.pr;

import javax.annotation.Nonnull;

import ru.yandex.ci.ydb.Persisted;

@Persisted
public enum ArcanumCheckType {
    CI_BUILD("ci", "build", "Build"),
    CI_BUILD_NATIVE("autocheck", "build_native", "Build [native]"),
    CI_TESTS("ci", "tests", "Style, Small & Medium Tests"),
    CI_LARGE_TESTS("ci", "large_tests", "Large Tests"),

    TE_JOBS("ci", "te_jobs", "TE Jobs"),
    AUTOCHECK_PESSIMIZED("autocheck", "pessimized", "https://docs.yandex-team.ru/ci/autocheck/pessimized");

    private final String system;
    private final String type;
    private final String description;

    ArcanumCheckType(@Nonnull String system, @Nonnull String type, @Nonnull String description) {
        this.system = system;
        this.type = type;
        this.description = description;
    }

    public static ArcanumCheckType of(String type) {
        for (var checkType : ArcanumCheckType.values()) {
            if (checkType.getType().equals(type)) {
                return checkType;
            }
        }
        throw new RuntimeException("Unknown check type " + type);
    }

    public String getSystem() {
        return system;
    }

    public String getType() {
        return type;
    }

    public String getDescription() {
        return description;
    }
}
