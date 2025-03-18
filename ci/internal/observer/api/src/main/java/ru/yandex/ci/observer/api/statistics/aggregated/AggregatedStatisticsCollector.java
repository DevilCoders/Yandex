package ru.yandex.ci.observer.api.statistics.aggregated;

import java.time.Instant;
import java.time.temporal.ChronoUnit;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;
import java.util.concurrent.TimeUnit;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;

import com.google.common.collect.Streams;
import io.micrometer.core.instrument.Gauge;
import io.micrometer.core.instrument.MeterRegistry;
import lombok.Getter;
import org.springframework.scheduling.annotation.Scheduled;

import ru.yandex.ci.observer.api.statistics.model.CheckTaskMaxDelay;
import ru.yandex.ci.observer.api.statistics.model.SensorLabels;
import ru.yandex.ci.observer.api.statistics.model.StageType;
import ru.yandex.ci.observer.api.statistics.model.StatisticsItem;
import ru.yandex.ci.observer.core.db.CiObserverDb;
import ru.yandex.ci.storage.core.CheckOuterClass.CheckType;


public class AggregatedStatisticsCollector {
    public static final String MAIN_STATISTICS_NAME = "main";
    public static final String ADDITIONAL_PERCENTILES_STATISTICS_NAME = "additional_percentiles";
    public static final String ADDITIONAL_STATISTICS_NAME = "additional";

    private static final String UPDATE_DELAY_SENSOR_NAME = "statistics_update_delay";
    private static final List<CheckType> MAIN_CHECK_TYPES = List.of(
            CheckType.TRUNK_PRE_COMMIT, CheckType.TRUNK_POST_COMMIT
    );
    private static final List<CheckType> ADDITIONAL_CHECK_TYPES = List.of(
            CheckType.BRANCH_PRE_COMMIT, CheckType.BRANCH_POST_COMMIT
    );
    private static final List<CheckType> TRUNK_AND_BRANCH_PRE_COMMITS = List.of(CheckType.BRANCH_PRE_COMMIT,
            CheckType.TRUNK_PRE_COMMIT);

    @Nonnull
    private final AggregatedStatisticsService aggregatedStats;
    @Nonnull
    private final CiObserverDb db;

    @Getter
    private volatile List<? extends StatisticsItem> mainStatisticsCache = List.of();
    @Getter
    private volatile List<? extends StatisticsItem> additionalStatisticsCache = List.of();
    @Getter
    private volatile List<? extends StatisticsItem> additionalPercentileStatisticsCache = List.of();
    @Getter
    private final ConcurrentMap<String, Instant> statisticsLastUpdateTime = new ConcurrentHashMap<>();

    public AggregatedStatisticsCollector(
            @Nonnull AggregatedStatisticsService aggregatedStats,
            @Nonnull MeterRegistry meterRegistry,
            @Nonnull CiObserverDb db
    ) {
        this.aggregatedStats = aggregatedStats;
        this.db = db;

        createStatisticsUpdateDelayMeter(MAIN_STATISTICS_NAME).register(meterRegistry);
        createStatisticsUpdateDelayMeter(ADDITIONAL_PERCENTILES_STATISTICS_NAME).register(meterRegistry);
        createStatisticsUpdateDelayMeter(ADDITIONAL_STATISTICS_NAME).register(meterRegistry);
    }

    @Scheduled(
            fixedDelayString = "${observer.AggregatedStatisticsCollector.refreshMainStatisticsSeconds}",
            timeUnit = TimeUnit.SECONDS
    )
    public void refreshMainStatistics() {
        var now = Instant.now().truncatedTo(ChronoUnit.SECONDS);
        var statisticsCollector = new ArrayList<StatisticsItem>();

        var preCommitFrom = now.minus(2, ChronoUnit.HOURS);
        statisticsCollector.addAll(aggregatedStats.getStagesPercentilesOfActiveIterations(
                preCommitFrom, now, CheckType.TRUNK_PRE_COMMIT, StageType.ANY
        ));
        statisticsCollector.addAll(aggregatedStats.getInflightIterationsCount(
                preCommitFrom, now, CheckType.TRUNK_PRE_COMMIT
        ));

        var postCommitFrom = now.minus(4, ChronoUnit.DAYS);
        statisticsCollector.addAll(aggregatedStats.getInflightIterationsCount(
                postCommitFrom, now, CheckType.TRUNK_POST_COMMIT
        ));
        statisticsCollector.addAll(CheckTaskMaxDelay.computeForPostCommits(postCommitFrom, now, db));

        statisticsCollector.addAll(aggregatedStats.getWindowedStartFinishIterationsCount(now, MAIN_CHECK_TYPES));
        statisticsCollector.addAll(aggregatedStats.getWindowedPassedFailedIterationsCount(now, MAIN_CHECK_TYPES));

        mainStatisticsCache = statisticsCollector;
        statisticsLastUpdateTime.put(MAIN_STATISTICS_NAME, now);
    }

    @Scheduled(
            fixedDelayString = "${observer.AggregatedStatisticsCollector.refreshAdditionalPercentileStatisticsSeconds}",
            timeUnit = TimeUnit.SECONDS
    )
    public void refreshAdditionalPercentileStatistics() {
        var now = Instant.now().truncatedTo(ChronoUnit.SECONDS);
        var from = now.minus(2, ChronoUnit.HOURS);

        additionalPercentileStatisticsCache = Streams.concat(
                //Trunk precommits
                aggregatedStats.getStagesPercentilesOfActiveIterations(
                        from, now,
                        CheckType.TRUNK_PRE_COMMIT,
                        StageType.ACTIVE
                ).stream(),
                aggregatedStats.getStagesPercentilesOfActiveIterations(
                        from, now,
                        CheckType.TRUNK_PRE_COMMIT,
                        StageType.WITH_ZEROES
                ).stream(),
                // Branch precommits
                aggregatedStats.getStagesPercentilesOfActiveIterations(
                        from, now,
                        CheckType.BRANCH_PRE_COMMIT,
                        StageType.ACTIVE
                ).stream(),
                aggregatedStats.getStagesPercentilesOfActiveIterations(
                        from, now,
                        CheckType.BRANCH_PRE_COMMIT,
                        StageType.ANY
                ).stream(),
                aggregatedStats.getStagesPercentilesOfActiveIterations(
                        from, now,
                        CheckType.BRANCH_PRE_COMMIT,
                        StageType.WITH_ZEROES
                ).stream(),
                // Trunk + branch precommits
                aggregatedStats.getStagesPercentilesOfActiveIterations(
                        from, now,
                        TRUNK_AND_BRANCH_PRE_COMMITS,
                        "ANY_PRE_COMMIT",
                        StageType.ACTIVE,
                        false
                ).stream(),
                aggregatedStats.getStagesPercentilesOfActiveIterations(
                        from, now,
                        TRUNK_AND_BRANCH_PRE_COMMITS,
                        "ANY_PRE_COMMIT",
                        StageType.ANY,
                        false
                ).stream(),
                aggregatedStats.getStagesPercentilesOfActiveIterations(
                        from, now,
                        TRUNK_AND_BRANCH_PRE_COMMITS,
                        "ANY_PRE_COMMIT",
                        StageType.WITH_ZEROES,
                        false
                ).stream()
        ).collect(Collectors.toList());

        statisticsLastUpdateTime.put(ADDITIONAL_PERCENTILES_STATISTICS_NAME, now);
    }

    @Scheduled(
            fixedDelayString = "${observer.AggregatedStatisticsCollector.refreshAdditionalStatisticsSeconds}",
            timeUnit = TimeUnit.SECONDS
    )
    public void refreshAdditionalStatistics() {
        Instant now = Instant.now().truncatedTo(ChronoUnit.SECONDS);

        additionalStatisticsCache = Streams.concat(
                aggregatedStats.getInflightIterationsCount(
                        now.minus(1, ChronoUnit.DAYS), now, ADDITIONAL_CHECK_TYPES
                ).stream(),
                aggregatedStats.getWindowedStartFinishIterationsCount(now, ADDITIONAL_CHECK_TYPES).stream(),
                aggregatedStats.getWindowedPassedFailedIterationsCount(now, ADDITIONAL_CHECK_TYPES).stream(),
                aggregatedStats.getInflightIterationsCountByCreateSystem(
                        now.minus(1, ChronoUnit.DAYS), now,
                        List.of(CheckType.TRUNK_PRE_COMMIT, CheckType.BRANCH_PRE_COMMIT)
                ).stream(),
                aggregatedStats.getWindowedStartFinishIterationsCountByCreateSystem(
                        now,
                        List.of(CheckType.TRUNK_PRE_COMMIT, CheckType.BRANCH_PRE_COMMIT)
                ).stream()
        ).collect(Collectors.toList());

        statisticsLastUpdateTime.put(ADDITIONAL_STATISTICS_NAME, now);
    }

    private Gauge.Builder<?> createStatisticsUpdateDelayMeter(String statisticsName) {
        return Gauge.builder(
                UPDATE_DELAY_SENSOR_NAME,
                () -> Instant.now().getEpochSecond()
                        - statisticsLastUpdateTime.getOrDefault(statisticsName, Instant.EPOCH).getEpochSecond()
        ).tag(SensorLabels.METRIC, "seconds").tag("statistics_name", statisticsName);
    }
}
