package ru.yandex.ci.storage.core.db.model.check_iteration;

import javax.annotation.Nullable;

import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Value;

import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.Common.ResultType;
import ru.yandex.ci.util.gson.PrettyJson;
import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value
@Builder
@AllArgsConstructor
// Using nullable fields to prevent empty statistics store in db and memory cache.
public class MainStatistics {
    public static final MainStatistics EMPTY = new MainStatistics(
            null, null, null, null, null, null, null, null, null
    );

    @Nullable
    StageStatistics total;

    @Nullable
    StageStatistics configure;

    @Nullable
    StageStatistics build;

    @Nullable
    StageStatistics style;

    @Nullable
    StageStatistics smallTests;

    @Nullable
    StageStatistics mediumTests;

    @Nullable
    StageStatistics largeTests;

    @Nullable
    StageStatistics teTests;

    @Nullable
    StageStatistics nativeBuilds;

    public StageStatistics getTotalOrEmpty() {
        return total == null ? StageStatistics.EMPTY : total;
    }


    public StageStatistics getConfigureOrEmpty() {
        return configure == null ? StageStatistics.EMPTY : configure;
    }

    public StageStatistics getBuildOrEmpty() {
        return build == null ? StageStatistics.EMPTY : build;
    }

    public StageStatistics getStyleOrEmpty() {
        return style == null ? StageStatistics.EMPTY : style;
    }

    public StageStatistics getSmallTestsOrEmpty() {
        return smallTests == null ? StageStatistics.EMPTY : smallTests;
    }

    public StageStatistics getMediumTestsOrEmpty() {
        return mediumTests == null ? StageStatistics.EMPTY : mediumTests;
    }


    public StageStatistics getLargeTestsOrEmpty() {
        return largeTests == null ? StageStatistics.EMPTY : largeTests;
    }

    public StageStatistics getTeTestsOrEmpty() {
        return teTests == null ? StageStatistics.EMPTY : teTests;
    }

    public StageStatistics getNativeBuildsOrEmpty() {
        return nativeBuilds == null ? StageStatistics.EMPTY : nativeBuilds;
    }

    public StageStatistics getByChunkTypeOrEmpty(Common.ChunkType type) {
        return switch (type) {
            case CT_CONFIGURE -> getConfigureOrEmpty();
            case CT_BUILD -> getBuildOrEmpty();
            case CT_STYLE -> getStyleOrEmpty();
            case CT_SMALL_TEST -> getSmallTestsOrEmpty();
            case CT_MEDIUM_TEST -> getMediumTestsOrEmpty();
            case CT_LARGE_TEST -> getLargeTestsOrEmpty();
            case CT_TESTENV -> getTeTestsOrEmpty();
            case CT_NATIVE_BUILD -> getNativeBuildsOrEmpty();
            case UNRECOGNIZED -> throw new IllegalArgumentException("Unrecognized result type " + type);
        };
    }

    public MainStatisticsMutable toMutable() {
        return new MainStatisticsMutable(
                StageStatistics.toMutable(total),
                StageStatistics.toMutable(configure),
                StageStatistics.toMutable(build),
                StageStatistics.toMutable(style),
                StageStatistics.toMutable(smallTests),
                StageStatistics.toMutable(mediumTests),
                StageStatistics.toMutable(largeTests),
                StageStatistics.toMutable(teTests),
                StageStatistics.toMutable(nativeBuilds)
        );
    }

    @Value
    public static class MainStatisticsMutable {
        StageStatistics.Mutable total;
        StageStatistics.Mutable configure;
        StageStatistics.Mutable build;
        StageStatistics.Mutable style;
        StageStatistics.Mutable smallTests;
        StageStatistics.Mutable mediumTests;
        StageStatistics.Mutable largeTests;
        StageStatistics.Mutable teTests;

        StageStatistics.Mutable nativeBuilds;

        public void add(MainStatisticsMutable statistics, int sign) {
            total.add(statistics.getTotal(), sign);
            configure.add(statistics.getConfigure(), sign);
            build.add(statistics.getBuild(), sign);
            style.add(statistics.getStyle(), sign);
            smallTests.add(statistics.getSmallTests(), sign);
            mediumTests.add(statistics.getMediumTests(), sign);
            largeTests.add(statistics.getLargeTests(), sign);
            teTests.add(statistics.getTeTests(), sign);
            nativeBuilds.add(statistics.getNativeBuilds(), sign);
        }

        public MainStatistics toImmutable() {
            return new MainStatistics(
                    total.equals(StageStatistics.Mutable.EMPTY) ? null : total.toImmutable(),
                    configure.equals(StageStatistics.Mutable.EMPTY) ? null : configure.toImmutable(),
                    build.equals(StageStatistics.Mutable.EMPTY) ? null : build.toImmutable(),
                    style.equals(StageStatistics.Mutable.EMPTY) ? null : style.toImmutable(),
                    smallTests.equals(StageStatistics.Mutable.EMPTY) ? null : smallTests.toImmutable(),
                    mediumTests.equals(StageStatistics.Mutable.EMPTY) ? null : mediumTests.toImmutable(),
                    largeTests.equals(StageStatistics.Mutable.EMPTY) ? null : largeTests.toImmutable(),
                    teTests.equals(StageStatistics.Mutable.EMPTY) ? null : teTests.toImmutable(),
                    nativeBuilds.equals(StageStatistics.Mutable.EMPTY) ? null : nativeBuilds.toImmutable()
            );
        }

        public StageStatistics.Mutable getStage(ResultType type) {
            return switch (type) {
                case RT_BUILD -> this.build;
                case RT_CONFIGURE -> this.configure;
                case RT_STYLE_CHECK, RT_STYLE_SUITE_CHECK -> this.style;
                case RT_TEST_LARGE, RT_TEST_SUITE_LARGE -> this.largeTests;
                case RT_TEST_MEDIUM, RT_TEST_SUITE_MEDIUM -> this.mediumTests;
                case RT_TEST_SMALL, RT_TEST_SUITE_SMALL -> this.smallTests;
                case RT_TEST_TESTENV -> this.teTests;
                case RT_NATIVE_BUILD -> this.nativeBuilds;
                case UNRECOGNIZED -> throw new RuntimeException("Unrecognized result type " + type);
            };
        }

        public boolean hasNegativeNumbers() {
            return
                    total.hasNegativeNumbers() ||
                            configure.hasNegativeNumbers() ||
                            build.hasNegativeNumbers() ||
                            style.hasNegativeNumbers() ||
                            smallTests.hasNegativeNumbers() ||
                            mediumTests.hasNegativeNumbers() ||
                            largeTests.hasNegativeNumbers() ||
                            teTests.hasNegativeNumbers() ||
                            nativeBuilds.hasNegativeNumbers();
        }
    }

    @Override
    public String toString() {
        return PrettyJson.format(this);
    }
}
