package ru.yandex.ci.common.application.profiles;

public class CiProfile {

    public static final String LOCAL_PROFILE = "local";
    public static final String UNIT_TEST_PROFILE = "unit-test";

    public static final String STABLE_PROFILE = "stable";
    public static final String PRESTABLE_PROFILE = "prestable";
    public static final String TESTING_PROFILE = "testing";

    public static final String NOT_UNIT_TEST_PROFILE = "!" + UNIT_TEST_PROFILE;

    public static final String NOT_TESTING_PROFILE = "!" + TESTING_PROFILE;

    public static final String NOT_STABLE_PROFILE = "!" + STABLE_PROFILE;

    public static final String STABLE_OR_TESTING_PROFILE = STABLE_PROFILE + "|" + TESTING_PROFILE;
    public static final String PRESTABLE_OR_TESTING_PROFILE = PRESTABLE_PROFILE + "|" + TESTING_PROFILE;
    public static final String NOT_STABLE_OR_TESTING_PROFILE = "!(" + STABLE_OR_TESTING_PROFILE + ")";

    public static final String TESTING_OR_LOCAL_PROFILE = TESTING_PROFILE + "|" + LOCAL_PROFILE;

    private CiProfile() {
    }
}
