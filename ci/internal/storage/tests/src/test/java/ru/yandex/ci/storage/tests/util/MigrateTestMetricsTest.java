package ru.yandex.ci.storage.tests.util;

import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.sql.SQLException;
import java.time.Instant;
import java.time.LocalDate;
import java.time.ZoneOffset;
import java.time.temporal.ChronoUnit;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.Set;
import java.util.function.Function;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import com.google.common.base.Splitter;
import lombok.extern.slf4j.Slf4j;
import org.junit.jupiter.api.Disabled;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.PropertySource;
import org.springframework.test.context.ActiveProfiles;
import org.springframework.test.context.ContextConfiguration;
import org.springframework.test.context.junit.jupiter.SpringExtension;

import yandex.cloud.repository.kikimr.yql.YqlLimit;
import yandex.cloud.repository.kikimr.yql.YqlOrderBy;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;
import yandex.cloud.repository.kikimr.yql.YqlView;
import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.common.ydb.YqlPredicateCi;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.CheckOuterClass;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.clickhouse.AutocheckClickhouse;
import ru.yandex.ci.storage.core.clickhouse.SpClickhouseConfig;
import ru.yandex.ci.storage.core.clickhouse.sp.Result;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.clickhouse.metrics.TestMetricEntity;
import ru.yandex.ci.storage.core.db.clickhouse.metrics.TestMetricTable;
import ru.yandex.ci.storage.core.db.clickhouse.old_test.OldTestEntity;
import ru.yandex.ci.storage.core.db.clickhouse.runs.TestRunEntity;
import ru.yandex.ci.storage.core.db.clickhouse.runs.TestRunTable;
import ru.yandex.ci.storage.core.db.clickhouse.runs.TestRunV2Entity;
import ru.yandex.ci.storage.core.db.clickhouse.runs.TestRunV2Table;
import ru.yandex.ci.storage.core.db.constant.Trunk;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.revision.RevisionEntity;
import ru.yandex.ci.storage.core.db.model.task_result.TestResultEntity;
import ru.yandex.ci.storage.core.db.model.test_status.TestStatusEntity;
import ru.yandex.ci.storage.core.test_metrics.MetricsClickhouseConfig;

@ExtendWith(SpringExtension.class)
@ContextConfiguration(classes = {
        ru.yandex.ci.storage.tests.ClickHouseTest.Config.class
})
@ActiveProfiles(profiles = CiProfile.STABLE_PROFILE)
@Slf4j
@Disabled
public class MigrateTestMetricsTest {
    private static final Set<String> PATHS = Set.of(
            "devtools/ya/build/tests/buids",
            "devtools/ymake/tools/uid_debugger/tests",
            "devtools/dummy_arcadia/flacky_metric",
            "devtools/ymake/tests/perf",
            "devtools/ymake/tests/perf_with_cache/json_cache",
            "devtools/ymake/tests/perf_with_cache/internal_cache",
            "devtools/ymake/tests/perf_with_cache/filesystem_cache",
            "devtools/ymake/tests/perf_callgrind",
            "devtools/ymake/tools/ygdiff/tests",
            "devtools/ymake/tools/ygdiff/tests/perf",
            "devtools/ymake/tools/contexts_difference_cpp/tests/perf",
            "devtools/ya/build/tests/perf",
            "devtools/ymake/tests/perf_with_changes"
    );

    @Autowired
    private AutocheckClickhouse clickhouse;

    @Autowired
    private TestRunTable testRunTable;

    @Autowired
    private TestRunV2Table testRunV2Table;

    @Autowired
    private TestMetricTable metricTable;

    @Autowired
    private CiStorageDb db;

    @Test
    @Disabled
    public void extractTestIds() throws IOException, SQLException, InterruptedException {
        log.info("Loading all tests");
        var predicate = YqlPredicateCi.in("path", PATHS)
                .and(
                        YqlPredicate
                                .where("type").eq(Common.ResultType.RT_TEST_LARGE)
                                .or("type").eq(Common.ResultType.RT_TEST_SUITE_LARGE)
                );
        var tests = db.scan().run(() -> db.tests().find(predicate));

        log.info("Found {} tests", tests.size());

        var oldTests = clickhouse.getTests(
                tests.stream().map(TestStatusEntity::getOldTestId).collect(Collectors.toSet())
        );

        log.info("Found {} old tests", oldTests.size());

        var strIdsFile = File.createTempFile("tests", ".txt");
        Files.writeString(
                strIdsFile.toPath(),
                oldTests.stream().map(x -> "%s,%d".formatted(x.getStrId(), x.getId())).collect(Collectors.joining("\n"))
        );
        log.info("Exported ids to {}", strIdsFile.getAbsolutePath());
    }

    @Test
    @Disabled
    public void migrateMetrics() throws SQLException, InterruptedException, IOException {
        //var lines = List.of("6a99edd545bc19950b8a356ead67837a,19085253");
        var lines = Files.readAllLines(
                Path.of("/var/folders/d6/hn2s8rw10hj6f57h9yd432_jmt2mb5/T/tests1184816900904805029.txt")
        );

        var now = LocalDate.now(ZoneOffset.UTC);
        var index = 0;
        for (var line : lines) {
            log.info("");
            var pair = Splitter.on(',').splitToList(line);
            var oldStrId = pair.get(0);
            var oldId = Long.parseLong(pair.get(1));

            var test = clickhouse.getTests(Set.of(oldStrId)).stream()
                    .filter(x -> x.getId() == oldId).findFirst().orElseThrow();
            log.info(
                    "Exporting path: {}\nname: {}, subtest name: {}, toolchain: {}\nstr id: {}, line: {} / {}",
                    test.getPath(), test.getName(), test.getSubtestName(), test.getToolchain(),
                    test.getStrId(), ++index, lines.size()
            );

            var runs = testRunTable.getRuns(oldId, now.minus(365, ChronoUnit.DAYS), now).stream()
                    .collect(Collectors.toMap(TestRunEntity::getId, Function.identity(), (a, b) -> b));

            var runsV2 = testRunV2Table.getRuns(oldId, now.minus(365, ChronoUnit.DAYS), now);
            var runsV2Matched = runsV2.stream()
                    .filter(x -> {
                        var run = runs.get(x.getRunId());
                        if (run == null) {
                            return false;
                        }
                        return run.getDate().equals(x.getDate());
                    }).toList();

            var revisionIds = runsV2Matched.stream()
                    .map(x -> new RevisionEntity.Id((long) (int) x.getRevision(), Trunk.name()))
                    .collect(Collectors.toSet());

            var revisions = db.currentOrReadOnly(() -> db.revisions().find(revisionIds)).stream().collect(
                    Collectors.toMap(x -> (int) (long) x.getId().getNumber(), RevisionEntity::getCreated)
            );
            log.info("Found {} runs, {} runs_v2, {} matched runs_v2", runs.size(), runsV2.size(), runsV2Matched.size());

            var metrics = runsV2Matched.stream()
                    .flatMap(x -> {
                        var revision = revisions.get(x.getRevision());
                        if (revision == null) {
                            log.info("Revision not found: {}", x.getRevision());
                            revision = x.getDate().atStartOfDay().toInstant(ZoneOffset.UTC);
                        }
                        return extractMetrics(test, x, runs.get(x.getRunId()), revision);
                    })
                    .toList();
            log.info("Extracted {} metrics", metrics.size());

            metricTable.save(metrics);
        }
    }

    private Stream<TestMetricEntity> extractMetrics(
            OldTestEntity test, TestRunV2Entity runV2, TestRunEntity run, Instant timestamp
    ) {
        var metrics = new ArrayList<TestMetricEntity>(run.getMetricKeys().size());
        for (var i = 0; i < run.getMetricKeys().size(); ++i) {
            metrics.add(
                    new TestMetricEntity(
                            Trunk.name(),
                            test.getPath(),
                            test.getName(),
                            test.getSubtestName(),
                            test.getToolchain(),
                            run.getMetricKeys().get(i),
                            timestamp,
                            (long) (int) runV2.getRevision(),
                            toResultType(test.getType(), run.getTestSize()),
                            run.getMetricValues().get(i),
                            toTestStatus(run.getStatus()),
                            0L,
                            0L,
                            0L
                    )
            );
        }

        return metrics.stream();
    }

    private Common.TestStatus toTestStatus(Result.Status status) {
        return switch (status) {
            case OK -> Common.TestStatus.TS_OK;
            case FAILED -> Common.TestStatus.TS_FAILED;
            case SKIPPED -> Common.TestStatus.TS_SKIPPED;
            case DISCOVERED -> Common.TestStatus.TS_DISCOVERED;
        };
    }

    private Common.ResultType toResultType(Result.TestType type, Result.TestSize testSize) {
        if (type == Result.TestType.BUILD) {
            return Common.ResultType.RT_BUILD;
        }

        if (type == Result.TestType.CONFIGURE) {
            return Common.ResultType.RT_CONFIGURE;
        }

        if (type == Result.TestType.STYLE) {
            return Common.ResultType.RT_STYLE_CHECK;
        }

        return switch (testSize) {
            case SMALL -> Common.ResultType.RT_TEST_SMALL;
            case MEDIUM -> Common.ResultType.RT_TEST_MEDIUM;
            case FAT, LARGE -> Common.ResultType.RT_TEST_LARGE;
            case TE_JOB -> Common.ResultType.RT_TEST_TESTENV;
        };
    }

    @Test
    @Disabled
    public void exportFromStorage() {
        var checks = db.currentOrReadOnly(() -> db.checks().find(
                YqlPredicate
                        .where("status").eq(Common.CheckStatus.COMPLETED_SUCCESS)
                        .and("created").gte(Instant.now().minus(10, ChronoUnit.DAYS))
                        .and("created").lt(Instant.now())
                        .and("type").eq(CheckOuterClass.CheckType.TRUNK_POST_COMMIT),
                YqlView.index(CheckEntity.IDX_BY_STATUS_AND_CREATED),
                YqlOrderBy.orderBy("status", "created"),
                YqlLimit.top(1000)
        ));

        var processedChecks = 0;
        var exportedChecks = 0;
        while (!checks.isEmpty()) {
            log.info("Found {} checks", checks.size());
            processedChecks += checks.size();

            var postCommitChecks = checks;
            var checkIds = postCommitChecks.stream().map(CheckEntity::getId).collect(Collectors.toSet());
            if (!checkIds.isEmpty()) {
                exportedChecks += checkIds.size();
                var iterations = db.currentOrReadOnly(() -> db.checkIterations().findByChecks(checkIds));

                iterations = iterations.stream()
                        .filter(x -> x.getId().getIterationType() == CheckIteration.IterationType.HEAVY)
                        .filter(x -> x.getId().getNumber() > 0)
                        .collect(Collectors.toList());

                log.info("Found {} iterations", iterations.size());

                for (var iteration : iterations) {
                    var predicate = YqlPredicateCi.in("path", PATHS)
                            .and(
                                    YqlPredicate
                                            .where("id.checkId").eq(iteration.getId().getCheckId().getId())
                                            .and("id.iterationType").eq(iteration.getId().getIterationTypeNumber())
                                            .and("id.iterationNumber").eq(iteration.getId().getNumber())
                                            .and("resultType").in(
                                                    Set.of(
                                                            Common.ResultType.RT_TEST_SUITE_LARGE,
                                                            Common.ResultType.RT_TEST_LARGE
                                                    )
                                            )
                            );
                    var results = db.scan().run(() -> db.testResults().find(predicate));
                    if (results.isEmpty()) {
                        log.info("No results in {}", iteration.getId());
                        continue;
                    } else {
                        log.info("Found {} results in {}", results.size(), iteration.getId());
                    }

                    var metrics = results.stream().flatMap(this::extractMetrics).toList();
                    log.info("Extracted {} metrics from {}", metrics.size(), iteration.getId());

                    metricTable.save(metrics);
                }
            }

            var lastCreated = checks.stream().max(Comparator.comparing(CheckEntity::getCreated)).get().getCreated();
            log.info("Last created: {}, processed: {}, exported: {}", lastCreated, processedChecks, exportedChecks);

            checks = db.currentOrReadOnly(() -> db.checks().find(
                    YqlPredicate
                            .where("status").eq(Common.CheckStatus.COMPLETED_SUCCESS)
                            .and("created").gt(lastCreated)
                            .and("created").lt(Instant.now())
                            .and("type").eq(CheckOuterClass.CheckType.TRUNK_POST_COMMIT),
                    YqlView.index(CheckEntity.IDX_BY_STATUS_AND_CREATED),
                    YqlOrderBy.orderBy("status", "created"),
                    YqlLimit.top(1000)
            ));
        }
    }

    private Stream<TestMetricEntity> extractMetrics(TestResultEntity result) {
        return result.getMetrics().entrySet().stream().map(
                entry -> new TestMetricEntity(
                        Trunk.name(),
                        result.getPath(),
                        result.getName(),
                        result.getSubtestName(),
                        result.getId().getToolchain(),
                        entry.getKey(),
                        result.getCreated(),
                        result.getRevisionNumber(),
                        result.getResultType(),
                        entry.getValue(),
                        result.getStatus(),
                        result.getId().getCheckId().getId(),
                        result.getId().getSuiteId(),
                        result.getId().getTestId()
                )
        );
    }

    @Configuration
    @PropertySource("classpath:ci-storage-core/ci-storage.properties")
    @PropertySource("classpath:ci-storage-core/ci-storage-stable.properties")
    @PropertySource(value = "file:${user.home}/.ci/ci-local.properties", ignoreResourceNotFound = true)
    @Import(value = {SpClickhouseConfig.class, MetricsClickhouseConfig.class})
    public static class Config {
    }
}

