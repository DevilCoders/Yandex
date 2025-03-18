package ru.yandex.ci.storage.post_processor.processing;

import java.time.Clock;
import java.util.ArrayList;
import java.util.List;
import java.util.Set;
import java.util.stream.Collectors;

import lombok.AllArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.core.project.AutocheckProject;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.constant.ResultTypeUtils;
import ru.yandex.ci.storage.core.db.model.test.TestTag;
import ru.yandex.ci.storage.core.db.model.test_launch.TestLaunchEntity;
import ru.yandex.ci.storage.core.db.model.test_mute.OldCiMuteActionEntity;
import ru.yandex.ci.storage.core.db.model.test_mute.TestMuteEntity;
import ru.yandex.ci.storage.core.db.model.test_statistics.TestStatisticsEntity;
import ru.yandex.ci.storage.core.db.model.test_status.TestStatistics;
import ru.yandex.ci.storage.core.db.model.test_status.TestStatusEntity;
import ru.yandex.ci.storage.core.utils.TimeTraceService;
import ru.yandex.ci.storage.post_processor.PostProcessorStatistics;
import ru.yandex.ci.storage.post_processor.cache.PostProcessorCache;
import ru.yandex.ci.util.HostnameUtils;

import static ru.yandex.ci.storage.core.Common.TestStatus.TS_FLAKY;
import static ru.yandex.ci.storage.core.Common.TestStatus.TS_NONE;

@Slf4j
@AllArgsConstructor
public class MessageProcessor {
    private static final int BULK_LIMIT = 8192;

    private static final Set<Common.TestStatus> IGNORED_STATUSES =
            Set.of(
                    Common.TestStatus.TS_DISCOVERED,
                    Common.TestStatus.TS_SKIPPED,
                    Common.TestStatus.TS_NOT_LAUNCHED
            );

    private final Clock clock;
    private final CiStorageDb db;
    private final PostProcessorCache cache;
    private final TimeTraceService timeTraceService;
    private final PostProcessorStatistics statistics;
    private final MuteSettings muteSettings;
    private final HistoryProcessor historyProcessor;
    private final MetricsProcessorPool metricsProcessorPool;

    public void process(List<ResultMessage> messages) {
        log.info("Processing {} result messages for post-processor", messages.size());

        var split = messages.stream().collect(
                Collectors.groupingBy(x -> IGNORED_STATUSES.contains(x.getResult().getStatus()))
        );

        processFiltered(split.getOrDefault(false, List.of()));
        split.getOrDefault(true, List.of()).forEach(x -> x.getCountdown().notifyMessageProcessed());
    }

    private void processFiltered(List<ResultMessage> messages) {
        var trace = timeTraceService.createTrace("message_processor");

        log.info("Warming cache");
        this.cache.tests().get(
                messages.stream()
                        .map(x -> toTestId(x.getResult()))
                        .collect(Collectors.toSet())
        );

        this.cache.testStatistics().get(
                messages.stream()
                        .map(x -> toTestStatisticsId(x.getResult()))
                        .collect(Collectors.toSet())
        );

        trace.step("cache_warm");

        log.info("Processing test statuses");
        this.cache.modify(cache -> processTestStatuses(messages, cache));

        trace.step("statuses");

        log.info("Processing test history");
        processTestHistory(messages);

        trace.step("history");

        log.info("Processing completed");

        messages.forEach(m -> m.getCountdown().notifyMessageProcessed("message_processor"));

        messages.forEach(metricsProcessorPool::enqueue);
    }

    private void processTestStatuses(List<ResultMessage> messages, PostProcessorCache.Modifiable cache) {
        var muteActions = new ArrayList<TestMuteEntity>();

        messages.forEach(message -> processResult(cache, message.getResult(), muteActions));

        var modified = new ArrayList<>(cache.tests().getAffected());
        this.db.currentOrTx(() -> this.db.tests().save(modified));

        var modifiedStatistics = new ArrayList<>(cache.testStatistics().getAffected());
        this.db.currentOrTx(
                () -> this.db.testStatistics().bulkUpsertWithRetries(
                        modifiedStatistics, BULK_LIMIT, (e) -> this.statistics.onBulkInsertError()
                )
        );

        this.db.currentOrTx(
                () -> {
                    this.db.testMutes().save(muteActions);
                    this.db.oldCiMuteActionTable().save(muteActions.stream().map(OldCiMuteActionEntity::of).toList());
                }
        );

        this.statistics.onTestsUpdated(modified.size());
    }

    private void processResult(
            PostProcessorCache.Modifiable cache,
            PostProcessorTestResult result,
            ArrayList<TestMuteEntity> muteActions
    ) {
        var buildOrConfigure = ResultTypeUtils.isBuildOrConfigure(result.getResultType());

        if (result.getStatus() == TS_NONE && buildOrConfigure) {
            // ignore build deletes
            return;
        }

        var test = cache.tests().getOrDefault(toTestId(result));
        var testStatistics = cache.testStatistics().getOrDefault(toTestStatisticsId(result));

        var testBuilder = test.toBuilder();
        var testStatisticsBuilder = testStatistics.toBuilder();

        if (result.getRevisionNumber() > test.getRevisionNumber()) {
            fillFromResult(testBuilder, result);
        }
        var updatedStatistics = testStatistics.getStatistics().onRun(result.getStatus(), result.getCreated());

        testStatisticsBuilder.statistics(updatedStatistics);

        if (!buildOrConfigure) {
            processAutoMute(result, muteActions, test, testBuilder, updatedStatistics);
        }

        if (!testBuilder.build().equals(test)) {
            cache.tests().put(testBuilder.updated(clock.instant()).build());
        }

        cache.testStatistics().put(testStatisticsBuilder.build());
    }

    private void processAutoMute(
            PostProcessorTestResult result,
            List<TestMuteEntity> muteActions,
            TestStatusEntity test,
            TestStatusEntity.Builder testBuilder,
            TestStatistics updatedStatistics
    ) {
        if (result.getStatus() == TS_FLAKY && !test.isMuted() && !TestTag.isExternal(test.getTags())) {
            var reason = "Flaky in iteration %s on revision %d".formatted(
                    result.getId().getIterationId(), result.getRevisionNumber()
            );

            log.info("Muting {}, reason: {}", test.getId(), reason);
            testBuilder.isMuted(true);

            muteActions.add(toMuteAction(result, test, true, reason));
        } else if (
                updatedStatistics.getNoneFlakyDays() >= this.muteSettings.getDaysWithoutFlaky() && test.isMuted()
        ) {
            var reason = "Unmute in iteration %s on revision %dm number of none flaky days: %d".formatted(
                    result.getId().getIterationId(), result.getRevisionNumber(),
                    updatedStatistics.getNoneFlakyDays()
            );
            log.info("Unmuting {}, reason: {}", test.getId(), reason);
            testBuilder.isMuted(false);

            muteActions.add(toMuteAction(result, test, false, reason));
        }
    }

    private TestMuteEntity toMuteAction(
            PostProcessorTestResult result, TestStatusEntity test, boolean muted, String reason
    ) {
        return TestMuteEntity.builder()
                .id(new TestMuteEntity.Id(test.getId(), clock.instant()))
                .iterationId(result.getId().getIterationId())
                .oldSuiteId(result.getOldSuiteId())
                .oldTestId(result.getOldTestId())
                .name(result.getName())
                .subtestName(result.getSubtestName())
                .path(result.getPath())
                .service(result.getService())
                .revisionNumber(result.getRevisionNumber())
                .muted(muted)
                .reason(reason)
                .resultType(result.getResultType())
                .build();
    }

    private void fillFromResult(TestStatusEntity.Builder testBuilder, PostProcessorTestResult result) {
        testBuilder
                .autocheckChunkId(result.getAutocheckChunkId())
                .type(result.getResultType())
                .oldSuiteId(result.getOldSuiteId())
                .oldTestId(result.getOldTestId())
                .path(result.getPath())
                .tags(result.getTags())
                .owners(result.getOwners())
                .uid(result.getUid())
                .subtestName(result.getSubtestName())
                .name(result.getName())
                .status(result.getStatus())
                .service(
                        result.getService().equals(AutocheckProject.NAME) &&
                                !testBuilder.getService().equals(AutocheckProject.NAME) &&
                                !testBuilder.getService().isEmpty() ?
                                testBuilder.getService() : result.getService()
                )
                .revision(result.getRevision())
                .revisionNumber(result.getRevisionNumber());
    }

    private TestStatusEntity.Id toTestId(PostProcessorTestResult result) {
        return TestStatusEntity.Id.idInBranch(result.getBranch(), result.getId().getFullTestId());
    }

    private TestStatisticsEntity.Id toTestStatisticsId(PostProcessorTestResult result) {
        return TestStatisticsEntity.Id.idInBranch(result.getBranch(), result.getId().getFullTestId());
    }

    private void processTestHistory(List<ResultMessage> results) {
        var postprocessorResults = results.stream().map(ResultMessage::getResult).toList();

        // ignore build deletes
        postprocessorResults = postprocessorResults.stream().filter(
                x -> !ResultTypeUtils.isBuildOrConfigure(x.getResultType()) || x.getStatus() != TS_NONE
        ).toList();

        var history = convertToHistory(postprocessorResults);
        historyProcessor.process(history);
    }

    private List<TestLaunchEntity> convertToHistory(List<PostProcessorTestResult> results) {
        return results.stream().map(this::toHistory).collect(Collectors.toList());
    }

    private TestLaunchEntity toHistory(PostProcessorTestResult result) {
        var launchRef = new TestLaunchEntity.LaunchRef(
                result.getId().getIterationId(),
                result.getId().getTaskId(),
                result.getId().getPartition(),
                result.getId().getRetryNumber()
        );

        var check = cache.checks().getOrThrow(result.getId().getIterationId().getCheckId());

        return new TestLaunchEntity(
                new TestLaunchEntity.Id(
                        new TestStatusEntity.Id(result.getBranch(), result.getId().getFullTestId()),
                        result.getRevisionNumber(),
                        launchRef
                ),
                result.getStatus(),
                result.getUid(),
                HostnameUtils.getShortHostname(),
                result.getCreated(),
                result.isRight() ? check.getRight().getTimestamp() : check.getLeft().getTimestamp()
        );
    }
}
