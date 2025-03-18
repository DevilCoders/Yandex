package ru.yandex.ci.observer.api.statistics.aggregated;

import java.time.Instant;
import java.time.temporal.ChronoUnit;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

import javax.annotation.Nullable;

import com.google.gson.Gson;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.ci.observer.api.ObserverApiYdbTestBase;
import ru.yandex.ci.observer.api.statistics.model.AggregatedStageDuration;
import ru.yandex.ci.observer.api.statistics.model.AggregatedWindowedStatistics;
import ru.yandex.ci.observer.api.statistics.model.InflightIterationsCount;
import ru.yandex.ci.observer.api.statistics.model.SensorLabels;
import ru.yandex.ci.observer.api.statistics.model.StageType;
import ru.yandex.ci.observer.api.statistics.model.StatisticsItem;
import ru.yandex.ci.observer.core.db.model.check.CheckEntity;
import ru.yandex.ci.observer.core.db.model.check_iterations.CheckIterationEntity;
import ru.yandex.ci.observer.core.db.model.traces.DurationStages;
import ru.yandex.ci.observer.core.utils.StageAggregationUtils;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.CheckOuterClass.CheckType;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.util.ResourceUtils;
import ru.yandex.ci.util.gson.CiGson;

import static org.assertj.core.api.Assertions.assertThat;
import static org.assertj.core.api.Assertions.registerFormatterForType;

class AggregatedStatisticsServiceTest extends ObserverApiYdbTestBase {
    private final Gson gson = CiGson.instance();

    @Autowired
    AggregatedStatisticsService service;

    @BeforeEach
    void setup() {
        clock.setTime(TIME);
    }

    @Test
    void getStagesPercentilesOfActiveIterations() {
        db.currentOrTx(() -> {
            List.of(
                    createIteration(100L, Instant.parse("2021-10-20T11:20:00Z"), true, Map.of()),
                    createIteration(101L, Instant.parse("2021-10-20T11:20:00Z"), true, Map.of()),
                    createIteration(102L, Instant.parse("2021-10-20T11:20:00Z"), true, Map.of()),
                    createIteration(103L, Instant.parse("2021-10-20T11:20:00Z"), true, Map.of())
            ).forEach(db.iterations()::save);

            var stagesDurationSeconds = Map.of(
                    StageAggregationUtils.PRE_CREATION_STAGE, 0L,
                    StageAggregationUtils.CREATION_STAGE, 10L,
                    StageAggregationUtils.CONFIGURE_STAGE, 60L,
                    StageAggregationUtils.SANDBOX_STAGE, 20L
            );
            List.of(
                    createIteration(1L, Instant.parse("2021-10-20T11:40:00Z"), false, stagesDurationSeconds),
                    createIteration(2L, Instant.parse("2021-10-20T11:40:00Z"), false, stagesDurationSeconds),
                    createIteration(3L, Instant.parse("2021-10-20T11:40:00Z"), false, stagesDurationSeconds),
                    createIteration(4L, Instant.parse("2021-10-20T11:50:00Z"), false, stagesDurationSeconds)
            ).forEach(db.iterations()::save);

            stagesDurationSeconds = Map.of(
                    StageAggregationUtils.PRE_CREATION_STAGE, 0L,
                    StageAggregationUtils.CREATION_STAGE, 20L,
                    StageAggregationUtils.CONFIGURE_STAGE, 30L,
                    StageAggregationUtils.SANDBOX_STAGE, 40L
            );
            List.of(
                    createIteration(5L, Instant.parse("2021-10-20T11:53:00Z"), false, stagesDurationSeconds),
                    createIteration(6L, Instant.parse("2021-10-20T11:53:00Z"), false, stagesDurationSeconds),
                    createIteration(7L, Instant.parse("2021-10-20T11:53:00Z"), false, stagesDurationSeconds),
                    createIteration(8L, Instant.parse("2021-10-20T11:53:00Z"), false, stagesDurationSeconds),
                    createIteration(9L, Instant.parse("2021-10-20T11:53:00Z"), false, stagesDurationSeconds),
                    createIteration(10L, Instant.parse("2021-10-20T11:53:00Z"), false, stagesDurationSeconds)
            ).forEach(db.iterations()::save);
        });

        var actualStats = service.getStagesPercentilesOfActiveIterations(
                clock.instant().minus(2, ChronoUnit.HOURS), clock.instant(),
                CheckType.TRUNK_PRE_COMMIT,
                StageType.ANY
        );

        assertThat(actualStats).contains(
                parseJsonFromFile(
                        "getStagesPercentilesOfActiveIterations_statistics.json",
                        AggregatedStageDuration[].class
                )
        );
    }

    @Test
    void getStagesPercentilesOfActiveIterations_shouldNotIncludeStressTestStats() {
        db.currentOrTx(() -> {
            db.iterations().save(
                    createIteration(1L, TIME.minus(1, ChronoUnit.HOURS), false, Map.of(
                            StageAggregationUtils.PRE_CREATION_STAGE, 10L,
                            StageAggregationUtils.CREATION_STAGE, 20L,
                            StageAggregationUtils.CONFIGURE_STAGE, 30L,
                            StageAggregationUtils.SANDBOX_STAGE, 40L
                    ))
            );
            db.iterations().save(
                    createStressTestIteration(2L, TIME.minus(1, ChronoUnit.HOURS), false, Map.of(
                            StageAggregationUtils.PRE_CREATION_STAGE, 100L,
                            StageAggregationUtils.CREATION_STAGE, 200L,
                            StageAggregationUtils.CONFIGURE_STAGE, 300L,
                            StageAggregationUtils.SANDBOX_STAGE, 400L
                    ))
            );
        });

        var actualStats = service.getStagesPercentilesOfActiveIterations(
                TIME.minus(2, ChronoUnit.HOURS), TIME,
                List.of(CheckType.TRUNK_PRE_COMMIT),
                CheckType.TRUNK_PRE_COMMIT.toString(),
                StageType.ANY,
                false
        );

        var configureStage = actualStats.stream()
                .filter(it -> it.getLabels().get(SensorLabels.PERCENTILE).equals("80"))
                .filter(it -> it.getLabels().get(SensorLabels.STAGE).equals("configure"))
                .filter(it -> it.getLabels().get(SensorLabels.ITERATION).equals("any"))
                .filter(it -> it.getLabels().get(SensorLabels.POOL).equals("ANY_POOL"))
                .toList();

        assertThat(configureStage)
                .hasSize(1)
                .first()
                .extracting(StatisticsItem::getValue)
                .isEqualTo(30.0);
    }

    @Test
    void getInflightIterationsCount() {
        var iterations = generateIterations(1, 10, TIME, false);
        db.tx(() -> {
            db.iterations().save(iterations);
            db.iterations().save(generateIterationsForCheck(11L, false, TIME.minus(3, ChronoUnit.HOURS), null));
            db.iterations().save(generateIterationsForCheck(12L, true, TIME.plus(1, ChronoUnit.MINUTES), null));
        });

        var actualStats = service.getInflightIterationsCount(
                TIME.minus(2, ChronoUnit.HOURS), TIME, CheckType.TRUNK_PRE_COMMIT
        );

        registerFormatterForType(InflightIterationsCount.class, gson::toJson);
        assertThat(actualStats).containsExactlyInAnyOrder(
                parseJsonFromFile("getInflightIterationsCount.json", InflightIterationsCount[].class)
        );
    }

    @Test
    void getInflightIterationsCount_withFastIterations() {
        var iterations = generateIterations(1, 10, TIME, true);
        db.tx(() -> {
            db.iterations().save(iterations);
            db.iterations().save(generateIterationsForCheck(11L, true, TIME.minus(3, ChronoUnit.HOURS), null));
            db.iterations().save(generateIterationsForCheck(12L, true, TIME.plus(1, ChronoUnit.MINUTES), null));
        });

        var actualStats = service.getInflightIterationsCount(
                TIME.minus(2, ChronoUnit.HOURS), TIME, CheckType.TRUNK_PRE_COMMIT
        );

        registerFormatterForType(InflightIterationsCount.class, gson::toJson);
        assertThat(actualStats).containsExactlyInAnyOrder(
                parseJsonFromFile("getInflightIterationsCount_withFastIterations.json", InflightIterationsCount[].class)
        );
    }

    @Test
    void getWindowedStartFinishIterationsCount_startedIterations() {
        db.tx(() -> {
            db.iterations().save(generateIterations(1, 6, TIME, false));
            db.iterations().save(generateIterations(6, 10, TIME.minus(2, ChronoUnit.MINUTES), false));
            db.iterations().save(generateIterations(10, 13, TIME.minus(6, ChronoUnit.MINUTES), false));
            db.iterations().save(generateIterations(13, 20, TIME.minus(16, ChronoUnit.MINUTES), true));
            db.iterations().save(generateIterations(20, 30, TIME.minus(2, ChronoUnit.HOURS), false));
        });

        var actualStats = service.getWindowedStartFinishIterationsCount(
                TIME, List.of(CheckType.TRUNK_PRE_COMMIT)
        );

        registerFormatterForType(AggregatedWindowedStatistics.class, gson::toJson);
        assertThat(actualStats).contains(
                parseJsonFromFile(
                        "getWindowedStartFinishIterationsCount_startedIterations_statistics.json",
                        AggregatedWindowedStatistics[].class
                )
        );
    }

    @Test
    void getWindowedStartFinishIterationsCount_finishedIterations() {
        var createdTime = TIME.minus(2, ChronoUnit.HOURS);
        db.tx(() -> {
            db.iterations().save(generateIterations(1, 6, createdTime, TIME, false));
            db.iterations().save(
                    generateIterations(6, 10, createdTime, TIME.minus(2, ChronoUnit.MINUTES), false)
            );
            db.iterations().save(
                    generateIterations(10, 13, createdTime, TIME.minus(6, ChronoUnit.MINUTES), false)
            );
            db.iterations().save(
                    generateIterations(13, 20, createdTime, TIME.minus(16, ChronoUnit.MINUTES), true)
            );
            db.iterations().save(
                    generateIterations(20, 30, createdTime, TIME.minus(2, ChronoUnit.HOURS), false)
            );
        });

        var actualStats = service.getWindowedStartFinishIterationsCount(
                TIME, List.of(CheckType.TRUNK_PRE_COMMIT)
        );

        registerFormatterForType(AggregatedWindowedStatistics.class, gson::toJson);
        assertThat(actualStats).contains(
                parseJsonFromFile(
                        "getWindowedStartFinishIterationsCount_finishedIterations_statistics.json",
                        AggregatedWindowedStatistics[].class
                )
        );
    }

    private <T> T parseJsonFromFile(String filePath, Class<T> clazz) {
        return gson.fromJson(ResourceUtils.textResource(filePath), clazz);
    }

    private List<CheckIterationEntity> generateIterations(
            int checkIdsFrom, int checkIdsTo, Instant created, boolean withFast
    ) {
        return generateIterations(checkIdsFrom, checkIdsTo, created, null, withFast);
    }

    private List<CheckIterationEntity> generateIterations(
            int checkIdsFrom, int checkIdsTo, Instant created, @Nullable Instant finish, boolean withFast
    ) {
        var res = new ArrayList<CheckIterationEntity>();
        for (int i = checkIdsFrom; i < checkIdsTo; ++i) {
            res.addAll(generateIterationsForCheck(i, withFast, created, finish));
        }

        return res;
    }

    private List<CheckIterationEntity> generateIterationsForCheck(
            long checkIdValue, boolean withFast, Instant created, @Nullable Instant finish
    ) {
        var checkId = CheckEntity.Id.of(checkIdValue);
        var res = new ArrayList<CheckIterationEntity>();
        if (withFast) {
            res.add(
                    SAMPLE_ITERATION.toBuilder()
                            .id(new CheckIterationEntity.Id(checkId, CheckIteration.IterationType.FAST, 1))
                            .created(created)
                            .finish(finish)
                            .status(finish == null ? Common.CheckStatus.RUNNING : Common.CheckStatus.COMPLETED_SUCCESS)
                            .build()
            );
        }

        res.add(
                SAMPLE_ITERATION.toBuilder()
                        .id(new CheckIterationEntity.Id(checkId, CheckIteration.IterationType.FULL, 1))
                        .created(created)
                        .finish(finish)
                        .status(finish == null ? Common.CheckStatus.RUNNING : Common.CheckStatus.COMPLETED_SUCCESS)
                        .build()
        );

        return res;
    }

    private static CheckIterationEntity createStressTestIteration(long checkId, Instant created, boolean pessimized,
                                                                  Map<String, Long> stagesDurationSeconds) {
        return createIteration(checkId, created, pessimized, true, stagesDurationSeconds);
    }

    private static CheckIterationEntity createIteration(long checkId, Instant created, boolean pessimized,
                                                        Map<String, Long> stagesDurationSeconds) {
        return createIteration(checkId, created, pessimized, false, stagesDurationSeconds);
    }

    private static CheckIterationEntity createIteration(long checkId, Instant created, boolean pessimized,
                                                        boolean stressTest,
                                                        Map<String, Long> stagesDurationSeconds) {
        return SAMPLE_ITERATION.toBuilder()
                .id(new CheckIterationEntity.Id(
                        CheckEntity.Id.of(checkId),
                        CheckIteration.IterationType.FULL,
                        1
                ))
                .left(null)
                .right(null)
                .created(created)
                .pessimized(pessimized)
                .stressTest(stressTest)
                .stagesAggregation(new DurationStages(stagesDurationSeconds))
                .build();
    }
}
