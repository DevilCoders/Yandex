package ru.yandex.ci.storage.core.clickhouse;

import java.sql.SQLException;
import java.time.LocalDate;
import java.time.ZoneOffset;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import lombok.extern.slf4j.Slf4j;

import yandex.cloud.repository.db.readtable.ReadTableParams;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.clickhouse.sp.Result;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.clickhouse.change_run.ChangeRunEntity;
import ru.yandex.ci.storage.core.db.clickhouse.last_run.LastRunEntity;
import ru.yandex.ci.storage.core.db.clickhouse.old_test.OldTestEntity;
import ru.yandex.ci.storage.core.db.clickhouse.run_link.RunLinkEntity;
import ru.yandex.ci.storage.core.db.clickhouse.runs.TestRunEntity;
import ru.yandex.ci.storage.core.db.clickhouse.test_event.TestEventEntity;
import ru.yandex.ci.storage.core.db.constant.ResultTypeUtils;
import ru.yandex.ci.storage.core.db.constant.Trunk;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.revision.RevisionEntity;
import ru.yandex.ci.storage.core.db.model.sequence.DbSequence;
import ru.yandex.ci.storage.core.db.model.task_result.TestResultEntity;
import ru.yandex.ci.storage.core.ydb.SequenceService;
import ru.yandex.ci.util.ObjectStore;

@Slf4j
public class ClickHouseExportServiceImpl implements ClickHouseExportService {
    private final CiStorageDb db;
    private final AutocheckClickhouse clickhouse;
    private final SequenceService sequenceService;
    private final int batchSize;

    public ClickHouseExportServiceImpl(
            CiStorageDb db, AutocheckClickhouse clickhouse,
            SequenceService sequenceService, int batchSize
    ) {
        this.db = db;
        this.clickhouse = clickhouse;
        this.sequenceService = sequenceService;
        this.batchSize = batchSize;
        sequenceService.init(DbSequence.RUN_ID, 2000000000000L);
        sequenceService.init(DbSequence.TEST_ID, 100000000000L);
    }

    @Override
    public void process(CheckIterationEntity.Id iterationId) {
        try {
            processInternal(iterationId);
        } catch (Exception e) {
            throw new RuntimeException("", e);
        }
    }

    private void processInternal(CheckIterationEntity.Id iterationId) throws SQLException, InterruptedException {
        var check = db.currentOrReadOnly(() -> db.checks().get(iterationId.getCheckId()));
        log.info("Check {}, status: {}", check.getId(), check.getStatus());

        var iteration = db.currentOrReadOnly(() -> db.checkIterations().get(iterationId));
        log.info("Iteration {}, status: {}", iteration.getId(), iteration.getStatus());

        var revision = db.currentOrReadOnly(
                () -> db.revisions().find(new RevisionEntity.Id(check.getRight().getRevisionNumber(), Trunk.name()))
        ).orElse(new RevisionEntity(Trunk.name(), check.getRight().getRevisionNumber()));

        var lastKey = getKey(iterationId, iterationId.getIterationType().getNumber());
        var toKey = getKey(iterationId, iterationId.getIterationType().getNumber() + 1);

        saveEvent(check, iteration, revision);

        var exported = new ObjectStore<>(0);
        while (true) {
            var fromKey = lastKey;
            log.info("Reading from {} to {}", fromKey, toKey);
            var results = loadResults(toKey, fromKey);

            if (results.isEmpty()) {
                log.info("No more results");
                break;
            }

            lastKey = results.get(results.size() - 1).getId();
            log.info("Read {} rows", results.size());
            exported.set(exported.get() + results.size());

            results = results.stream().filter(TestResultEntity::isRight).collect(Collectors.toList());

            log.info("{} right results", results.size());

            if (results.isEmpty()) {
                continue;
            }

            var existingTests = clickhouse.getTests(
                    results.stream().map(TestResultEntity::getOldTestId).collect(Collectors.toSet())
            ).stream().collect(Collectors.groupingBy(OldTestEntity::getStrId, HashMap::new, Collectors.toList()));

            log.info("Loaded {} tests", existingTests.size());

            var runIds = sequenceService.next(DbSequence.RUN_ID, results.size());
            log.info("Run id start: {}", runIds.get(0));

            var newTests = new ArrayList<OldTestEntity>();
            var missingTestResults = getMissingTestResults(results, existingTests);

            log.info("Missing tests: {}", missingTestResults.size());
            if (!missingTestResults.isEmpty()) {
                var testIds = sequenceService.next(DbSequence.TEST_ID, missingTestResults.size());
                for (var i = 0; i < missingTestResults.size(); i++) {
                    var result = missingTestResults.get(i);
                    var id = testIds.get(i);
                    var test = new OldTestEntity(
                            LocalDate.now(ZoneOffset.UTC),
                            id,
                            result.getOldTestId(),
                            result.getId().getToolchain(),
                            convertToType(result.getResultType()),
                            result.getPath(),
                            result.getName(),
                            result.getSubtestName(),
                            ResultTypeUtils.isSuite(result.getResultType()),
                            ""
                    );
                    newTests.add(test);
                    var toolchains = existingTests.get(result.getOldTestId());
                    if (toolchains == null) {
                        existingTests.put(result.getOldTestId(), new ArrayList<>(List.of(test)));
                    } else {
                        existingTests.get(result.getOldTestId()).add(test);
                    }
                }
            }

            var changeRuns = new ArrayList<ChangeRunEntity>(results.size());
            var runs = new ArrayList<TestRunEntity>(results.size());
            var lastRuns = new ArrayList<LastRunEntity>(results.size());
            var runLinks = new ArrayList<RunLinkEntity>(results.size());

            for (var i = 0; i < results.size(); i++) {
                var result = results.get(i);
                var toolchains = existingTests.get(result.getOldTestId());

                var test = toolchains.stream().filter(x -> x.getToolchain().equals(result.getId().getToolchain()))
                        .findFirst().orElseThrow(() -> new RuntimeException("Test not found: " + result.getId()));

                var timestamp = LocalDate.now(ZoneOffset.UTC);
                var status = convert(result.getStatus());
                var errorType = convertErrorType(result.getStatus());
                changeRuns.add(convertToChangeRun(check, runIds, i, test, timestamp, status, errorType));

                var runId = runIds.get(i);
                var metricKeys = new ArrayList<String>(result.getMetrics().size());
                var metricValues = new ArrayList<Double>(result.getMetrics().size());
                result.getMetrics().forEach((key, value) -> {
                    metricKeys.add(key);
                    metricValues.add(value);
                });

                var linksKeys = new ArrayList<String>(result.getLinks().size());
                var linksValues = new ArrayList<String>(result.getLinks().size());
                result.getLinks().forEach((key, value) -> value.forEach(link -> {
                            linksKeys.add(key);
                            linksValues.add(link);
                        }
                ));

                var snippet = "%s\n---\nExported from new CI: %d/%s".formatted(
                        result.getSnippet(), check.getId().getId(), iteration.getId().getIterationType()
                );

                runs.add(
                        convertToRun(
                                result, test, timestamp, status, errorType, runId, metricKeys, metricValues, snippet
                        )
                );

                lastRuns.add(
                        convert(
                                check, result, test, timestamp, status, errorType, runId, metricKeys, metricValues,
                                linksKeys, linksValues, snippet, result.getStatus()
                        )
                );

                runLinks.add(new RunLinkEntity(timestamp, runId, test.getId(), linksKeys, linksValues));
            }

            clickhouse.insert(newTests, runs, changeRuns, lastRuns, runLinks);
        }

        log.info("Exported: {}", exported.get());
    }

    private void saveEvent(CheckEntity check, CheckIterationEntity iteration, RevisionEntity revision)
            throws InterruptedException {
        // var tasks = db.currentOrReadOnly(() -> db.checkTasks().getByIteration(iterationId));
        var event = new TestEventEntity(
                check.getRight().getTimestamp(),
                (int) check.getRight().getRevisionNumber(),
                2,
                check.getAuthor(),
                revision.getMessage(),
                Set.of(),
                Set.of(),
                /* tasks.stream()
                        .map(CheckTaskEntity::getJobName)
                        .distinct()
                        .collect(Collectors.toList()), // this cause troubles
                tasks.stream()
                        .flatMap(x -> x.getCompletedPartitions().stream())
                        .distinct()
                        .collect(Collectors.toList()),
                 */
                List.of(),
                List.of(),
                List.of(),
                List.of(),
                iteration.getInfo().getAdvisedPool()
        );

        clickhouse.insert(List.of(event));
    }

    private TestRunEntity convertToRun(
            TestResultEntity result, OldTestEntity test, LocalDate timestamp, Result.Status status,
            Result.ErrorType errorType, Long runId, ArrayList<String> metricKeys,
            ArrayList<Double> metricValues, String snippet
    ) {
        return new TestRunEntity(
                timestamp,
                runId,
                0,
                test.getId(),
                status,
                errorType,
                result.getOwners().getGroups(),
                result.getOwners().getLogins(),
                result.getUid(),
                snippet,
                1.0,
                convert(result.getResultType()),
                new ArrayList<>(result.getTags()),
                result.getRequirements(),
                metricKeys,
                metricValues,
                ""
        );
    }

    private ChangeRunEntity convertToChangeRun(
            CheckEntity check,
            List<Long> runIds,
            int i,
            OldTestEntity test,
            LocalDate timestamp, Result.Status status,
            Result.ErrorType errorType
    ) {
        return new ChangeRunEntity(
                timestamp,
                test.getId(),
                (int) check.getRight().getRevisionNumber(),
                2,
                runIds.get(i),
                status,
                errorType,
                true,
                0,
                true,
                false
        );
    }

    private Result.TestType convertToType(Common.ResultType resultType) {
        return switch (resultType) {
            case RT_BUILD -> Result.TestType.BUILD;
            case RT_CONFIGURE -> Result.TestType.CONFIGURE;
            case RT_STYLE_CHECK, RT_STYLE_SUITE_CHECK -> Result.TestType.STYLE;
            case RT_TEST_LARGE, RT_TEST_MEDIUM, RT_TEST_SMALL, RT_TEST_SUITE_LARGE,
                    RT_TEST_SUITE_MEDIUM, RT_TEST_SUITE_SMALL, RT_TEST_TESTENV,
                    RT_NATIVE_BUILD, UNRECOGNIZED -> Result.TestType.TEST;
        };
    }

    private List<TestResultEntity> getMissingTestResults(
            List<TestResultEntity> results,
            Map<String, List<OldTestEntity>> existingTests
    ) {
        return results.stream().filter(result -> {
            var toolchains = existingTests.get(result.getOldTestId());
            if (toolchains == null) {
                return true;
            }
            var test = toolchains.stream().filter(x -> x.getToolchain().equals(result.getId().getToolchain()))
                    .findFirst().orElse(null);

            return test == null;

        }).toList();
    }

    private LastRunEntity convert(
            CheckEntity check,
            TestResultEntity result,
            OldTestEntity test,
            LocalDate timestamp,
            Result.Status status,
            Result.ErrorType errorType,
            Long runId,
            ArrayList<String> metricKeys,
            ArrayList<Double> metricValues,
            ArrayList<String> linksKeys,
            ArrayList<String> linksValues,
            String snippet,
            Common.TestStatus originalStatus
    ) {
        return new LastRunEntity(
                timestamp,
                originalStatus != Common.TestStatus.TS_NONE,
                test.getId(),
                2,
                (int) check.getRight().getRevisionNumber(),
                runId,
                0,
                test.getStrId(),
                result.getId().getToolchain(),
                Result.TestType.TEST,
                result.getPath(),
                result.getName(),
                result.getSubtestName(),
                ResultTypeUtils.isSuite(result.getResultType()),
                "",
                status,
                errorType,
                result.getOwners().getGroups(),
                result.getOwners().getLogins(),
                result.getUid(),
                snippet,
                0.0,
                convert(result.getResultType()),
                new ArrayList<>(result.getTags()),
                result.getRequirements(),
                linksKeys,
                linksValues,
                metricKeys,
                metricValues,
                ""
        );
    }

    private TestResultEntity.Id getKey(CheckIterationEntity.Id iterationId, int iterationType) {
        return new TestResultEntity.Id(
                iterationId.getCheckId(), iterationType, null, null, null, null, null, null, null
        );
    }

    private List<TestResultEntity> loadResults(TestResultEntity.Id toKey, TestResultEntity.Id fromKey) {
        return db.currentOrReadOnly(
                () -> db.testResults()
                        .readTable(
                                ReadTableParams.<TestResultEntity.Id>builder()
                                        .fromKey(fromKey)
                                        .toKey(toKey)
                                        .fromInclusive(false)
                                        .toInclusive(false)
                                        .rowLimit(batchSize)
                                        .ordered()
                                        .build()
                        ).toList()
        );
    }

    private Result.TestSize convert(Common.ResultType resultType) {
        return switch (resultType) {
            case RT_BUILD, RT_CONFIGURE, RT_STYLE_CHECK, RT_STYLE_SUITE_CHECK, UNRECOGNIZED -> null;
            case RT_TEST_LARGE, RT_TEST_SUITE_LARGE, RT_NATIVE_BUILD -> Result.TestSize.LARGE;
            case RT_TEST_MEDIUM, RT_TEST_SUITE_MEDIUM -> Result.TestSize.MEDIUM;
            case RT_TEST_SMALL, RT_TEST_SUITE_SMALL -> Result.TestSize.SMALL;
            case RT_TEST_TESTENV -> Result.TestSize.TE_JOB;
        };
    }

    @Nullable
    private Result.ErrorType convertErrorType(Common.TestStatus status) {
        return switch (status) {
            case TS_UNKNOWN, UNRECOGNIZED, TS_NOT_LAUNCHED, TS_MULTIPLE_PROBLEMS,
                    TS_SUITE_PROBLEMS, TS_NONE, TS_SKIPPED, TS_OK, TS_DISCOVERED -> null;
            case TS_FAILED -> Result.ErrorType.REGULAR;
            case TS_FLAKY -> Result.ErrorType.FLAKY;
            case TS_INTERNAL -> Result.ErrorType.INTERNAL;
            case TS_TIMEOUT -> Result.ErrorType.TIMEOUT;
            case TS_BROKEN_DEPS -> Result.ErrorType.BROKEN_DEPS;
            case TS_XFAILED -> Result.ErrorType.XFAILED;
            case TS_XPASSED -> Result.ErrorType.XPASSED;
        };
    }

    private Result.Status convert(Common.TestStatus status) {
        return switch (status) {
            case TS_UNKNOWN, TS_SKIPPED, TS_SUITE_PROBLEMS,
                    TS_MULTIPLE_PROBLEMS, TS_NOT_LAUNCHED, UNRECOGNIZED -> Result.Status.SKIPPED;
            case TS_DISCOVERED -> Result.Status.DISCOVERED;
            case TS_FAILED, TS_FLAKY, TS_INTERNAL, TS_TIMEOUT, TS_BROKEN_DEPS, TS_XPASSED -> Result.Status.FAILED;
            case TS_OK, TS_XFAILED, TS_NONE -> Result.Status.OK;
        };
    }
}
