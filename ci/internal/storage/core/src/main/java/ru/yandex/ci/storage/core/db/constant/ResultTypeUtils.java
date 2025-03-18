package ru.yandex.ci.storage.core.db.constant;

import java.util.Map;
import java.util.Set;

import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.Common.ResultType;

public class ResultTypeUtils {
    public static final Set<ResultType> NOT_CHILD_TYPE = Set.of(
            ResultType.RT_BUILD,
            ResultType.RT_CONFIGURE,
            ResultType.RT_STYLE_SUITE_CHECK,
            ResultType.RT_TEST_SUITE_LARGE,
            ResultType.RT_TEST_SUITE_MEDIUM,
            ResultType.RT_TEST_SUITE_SMALL,
            ResultType.RT_TEST_TESTENV
    );

    private static final Map<ResultType, ResultType> TO_SUITE_MAP = Map.ofEntries(
            Map.entry(ResultType.RT_BUILD, ResultType.RT_BUILD),
            Map.entry(ResultType.RT_CONFIGURE, ResultType.RT_CONFIGURE),
            Map.entry(ResultType.RT_STYLE_CHECK, ResultType.RT_STYLE_SUITE_CHECK),
            Map.entry(ResultType.RT_STYLE_SUITE_CHECK, ResultType.RT_STYLE_SUITE_CHECK),
            Map.entry(ResultType.RT_TEST_LARGE, ResultType.RT_TEST_SUITE_LARGE),
            Map.entry(ResultType.RT_TEST_MEDIUM, ResultType.RT_TEST_SUITE_MEDIUM),
            Map.entry(ResultType.RT_TEST_SMALL, ResultType.RT_TEST_SUITE_SMALL),
            Map.entry(ResultType.RT_TEST_SUITE_LARGE, ResultType.RT_TEST_SUITE_LARGE),
            Map.entry(ResultType.RT_TEST_SUITE_MEDIUM, ResultType.RT_TEST_SUITE_MEDIUM),
            Map.entry(ResultType.RT_TEST_SUITE_SMALL, ResultType.RT_TEST_SUITE_SMALL),
            Map.entry(ResultType.RT_TEST_TESTENV, ResultType.RT_TEST_TESTENV),
            Map.entry(ResultType.RT_NATIVE_BUILD, ResultType.RT_NATIVE_BUILD)
    );

    private static final Map<ResultType, ResultType> TO_CHILD_MAP = Map.ofEntries(
            Map.entry(ResultType.RT_BUILD, ResultType.RT_BUILD),
            Map.entry(ResultType.RT_CONFIGURE, ResultType.RT_CONFIGURE),
            Map.entry(ResultType.RT_STYLE_SUITE_CHECK, ResultType.RT_STYLE_CHECK),
            Map.entry(ResultType.RT_STYLE_CHECK, ResultType.RT_STYLE_CHECK),
            Map.entry(ResultType.RT_TEST_SUITE_LARGE, ResultType.RT_TEST_LARGE),
            Map.entry(ResultType.RT_TEST_SUITE_MEDIUM, ResultType.RT_TEST_MEDIUM),
            Map.entry(ResultType.RT_TEST_SUITE_SMALL, ResultType.RT_TEST_SMALL),
            Map.entry(ResultType.RT_TEST_LARGE, ResultType.RT_TEST_SUITE_LARGE),
            Map.entry(ResultType.RT_TEST_MEDIUM, ResultType.RT_TEST_SUITE_MEDIUM),
            Map.entry(ResultType.RT_TEST_SMALL, ResultType.RT_TEST_SUITE_SMALL),
            Map.entry(ResultType.RT_TEST_TESTENV, ResultType.RT_TEST_TESTENV)
    );

    private static final Set<ResultType> SUITES = Set.of(
            ResultType.RT_STYLE_SUITE_CHECK,
            ResultType.RT_TEST_SUITE_LARGE,
            ResultType.RT_TEST_SUITE_MEDIUM,
            ResultType.RT_TEST_SUITE_SMALL
    );

    private static final Set<ResultType> TESTS = Set.of(
            ResultType.RT_TEST_LARGE,
            ResultType.RT_TEST_MEDIUM,
            ResultType.RT_TEST_SMALL,
            ResultType.RT_STYLE_CHECK
    );

    private ResultTypeUtils() {

    }

    public static boolean isSuite(ResultType value) {
        return SUITES.contains(value);
    }

    public static boolean isTest(ResultType value) {
        return TESTS.contains(value);
    }

    public static boolean isBuild(ResultType value) {
        return value.equals(ResultType.RT_BUILD);
    }

    public static boolean isConfigure(ResultType value) {
        return value.equals(ResultType.RT_CONFIGURE);
    }

    public static boolean isBuildOrConfigure(ResultType value) {
        return isBuild(value) || isConfigure(value);
    }

    public static boolean isStyle(ResultType value) {
        return value.equals(ResultType.RT_STYLE_CHECK);
    }

    public static boolean isStyleSuite(ResultType value) {
        return value.equals(ResultType.RT_STYLE_SUITE_CHECK);
    }

    public static ResultType toSuiteType(ResultType value) {
        return TO_SUITE_MAP.get(value);
    }

    public static ResultType toChildType(ResultType value) {
        return TO_CHILD_MAP.get(value);
    }

    public static Common.ChunkType toChunkType(ResultType resultType) {
        return switch (resultType) {
            case RT_BUILD -> Common.ChunkType.CT_BUILD;
            case RT_CONFIGURE -> Common.ChunkType.CT_CONFIGURE;
            case RT_STYLE_CHECK, RT_STYLE_SUITE_CHECK -> Common.ChunkType.CT_STYLE;
            case RT_TEST_LARGE, RT_TEST_SUITE_LARGE -> Common.ChunkType.CT_LARGE_TEST;
            case RT_TEST_MEDIUM, RT_TEST_SUITE_MEDIUM -> Common.ChunkType.CT_MEDIUM_TEST;
            case RT_TEST_SMALL, RT_TEST_SUITE_SMALL -> Common.ChunkType.CT_SMALL_TEST;
            case RT_TEST_TESTENV -> Common.ChunkType.CT_TESTENV;
            case RT_NATIVE_BUILD -> Common.ChunkType.CT_NATIVE_BUILD;
            case UNRECOGNIZED -> throw new RuntimeException();
        };
    }

    public static String toTestSize(ResultType resultType) {
        return switch (resultType) {
            case RT_TEST_LARGE, RT_TEST_SUITE_LARGE -> "large";
            case RT_TEST_MEDIUM, RT_TEST_SUITE_MEDIUM -> "medium";
            case RT_TEST_SMALL, RT_TEST_SUITE_SMALL -> "small";
            case RT_BUILD, RT_CONFIGURE, RT_STYLE_CHECK,
                    RT_STYLE_SUITE_CHECK, RT_TEST_TESTENV, RT_NATIVE_BUILD,
                    UNRECOGNIZED -> throw new RuntimeException();
        };
    }
}
