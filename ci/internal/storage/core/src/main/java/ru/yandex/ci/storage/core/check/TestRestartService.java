package ru.yandex.ci.storage.core.check;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.function.Function;
import java.util.stream.Collectors;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.google.common.base.Preconditions;
import lombok.AllArgsConstructor;
import lombok.Data;
import lombok.Value;
import lombok.extern.slf4j.Slf4j;
import org.apache.commons.collections4.ListUtils;

import ru.yandex.ci.api.internal.frontend.flow.oncommit.FrontendOnCommitFlowLaunchApi;
import ru.yandex.ci.api.storage.StorageApi;
import ru.yandex.ci.client.ci.CiClient;
import ru.yandex.ci.core.autocheck.AutocheckConstants;
import ru.yandex.ci.core.autocheck.FlowVars;
import ru.yandex.ci.core.flow.CiActionReference;
import ru.yandex.ci.core.proto.CiCoreProtoMappers;
import ru.yandex.ci.core.storage.StorageUtils;
import ru.yandex.ci.storage.api.StorageFrontApi;
import ru.yandex.ci.storage.core.CheckOuterClass;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.constant.Trunk;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.check_task.CheckTaskEntity;
import ru.yandex.ci.storage.core.db.model.check_task.ExpectedTask;
import ru.yandex.ci.storage.core.db.model.suite_restart.SuiteRestartEntity;
import ru.yandex.ci.storage.core.db.model.task_result.TestResultEntity;
import ru.yandex.ci.storage.core.db.model.test_diff.SuiteSearchFilters;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffImportantEntity;
import ru.yandex.ci.storage.core.logbroker.event_producer.StorageEventsProducer;
import ru.yandex.ci.storage.core.proto.CheckProtoMappers;
import ru.yandex.ci.util.CiJson;

@Value
@Slf4j
public class TestRestartService {
    private static final String YAML_DIR = "autocheck";
    private static final String PRECOMMIT_TRUNK_ACTION = "autocheck-trunk-precommits-recheck";
    private static final String PRECOMMIT_BRANCH_ACTION = "autocheck-branch-precommits-recheck";
    private static final String POSTCOMMIT_TRUNK_ACTION = "autocheck-trunk-postcommits-recheck";
    private static final String POSTCOMMIT_BRANCH_ACTION = "autocheck-branch-postcommits-recheck";

    CiStorageDb db;
    StorageEventsProducer storageEventsProducer;
    CiClient ciClient;

    int maxRecheckableTargets;

    public void run(CheckIterationEntity.Id iterationId) {
        var context = db.currentOrReadOnly(() -> this.createContext(iterationId));

        var iterationIdToSearch = context.getOriginalIterationId().getNumber() == 1 ?
                context.getOriginalIterationId() : context.getOriginalIterationId().toMetaId();

        log.info("Collection failed tests for {}", context.getOriginalIterationId());

        // single result type search works faster
        var suitesWithFailedTests = findSuitesForRestart(iterationIdToSearch);

        log.info(
                "Found {} tests in iteration {} for restart in {}",
                suitesWithFailedTests.size(), iterationIdToSearch, context.getRestartIterationId()
        );

        if (suitesWithFailedTests.isEmpty()) {
            return;
        }

        // Loading all suite runs on all platforms (cheaper yql query)
        var suiteRuns = loadSuiteRuns(
                context,
                suitesWithFailedTests.stream().map(x -> x.getId().getSuiteId()).collect(Collectors.toList())
        );

        log.info("Found {} suite runs", suiteRuns.size());

        var runsByTestIdAndRight = suiteRuns.stream().collect(
                Collectors.groupingBy(
                        x -> x.getId().getFullTestId(),
                        Collectors.mapping(Function.identity(), Collectors.groupingBy(TestResultEntity::isRight))
                )
        );

        var suiteRestarts = new ArrayList<SuiteRestartEntity>(suitesWithFailedTests.size() * 2);
        for (var suite : suitesWithFailedTests) {
            var runs = runsByTestIdAndRight.get(suite.getId().getCombinedTestId());
            var leftRuns = runs.get(false);
            var rightRuns = runs.get(true);

            if (leftRuns != null) {
                suiteRestarts.add(createSuiteRestart(iterationId, context, suite, leftRuns, false));
            }

            if (rightRuns != null) {
                suiteRestarts.add(createSuiteRestart(iterationId, context, suite, rightRuns, true));
            }
        }

        this.db.currentOrReadOnly(
                () -> this.db.suiteRestarts().bulkUpsertWithRetries(
                        suiteRestarts, 1000, e -> log.error("Failed to bulk insert", e)
                )
        );

        var expectedTasks = suiteRestarts.stream()
                .map(
                        suite -> new ExpectedTask(
                                StorageUtils.toRestartJobName(suite.getJobName()), suite.getId().isRight()
                        )
                )
                .collect(Collectors.toSet());

        this.db.currentOrTx(() -> {
            var restartIteration = this.db.checkIterations().get(context.getRestartIterationId())
                    .withExpectedTasks(expectedTasks);

            var metaIteration = this.db.checkIterations().get(restartIteration.getId().toMetaId());
            var expectedTasksForMeta = new HashSet<>(metaIteration.getExpectedTasks());
            expectedTasksForMeta.addAll(expectedTasks);

            this.db.checkIterations().save(restartIteration);
            this.db.checkIterations().save(metaIteration.withExpectedTasks(expectedTasksForMeta));

        });

        var flow = startFlow(context);
        var flowFullId = CiCoreProtoMappers.toFlowFullId(flow.getLaunch().getFlowProcessId());

        log.info("Started flow: {}/{}", flowFullId, flow.getLaunch().getNumber());

        this.db.currentOrTx(() -> {
            var restartIteration = this.db.checkIterations().get(context.getRestartIterationId());
            var actionRef = new CiActionReference(flowFullId, flow.getLaunch().getNumber());
            restartIteration = restartIteration.withInfo(
                    restartIteration.getInfo().toBuilder()
                            .ciActionReferences(List.of(actionRef))
                            .build()
            );

            var metaIteration = this.db.checkIterations().get(context.getRestartIterationId().toMetaId());
            metaIteration = metaIteration.withInfo(
                    metaIteration.getInfo().toBuilder()
                            .ciActionReferences(
                                    ListUtils.union(metaIteration.getInfo().getCiActionReferences(), List.of(actionRef))
                            )
                            .build()
            );


            this.db.checkIterations().save(restartIteration);
            this.db.checkIterations().save(metaIteration);
        });

        this.db.currentOrReadOnly(() -> storageEventsProducer.onIterationRegistered(
                CheckProtoMappers.toProtoCheck(db.checks().get(context.getCheck().getId())),
                CheckProtoMappers.toProtoIteration(db.checkIterations().get(context.restartIterationId))
        ));
    }

    private ru.yandex.ci.api.proto.Common.OrderedArcRevision lookupConfigRevision() {
        var request = StorageApi.GetLastValidConfigRequest.newBuilder()
                .setDir(YAML_DIR)
                .setBranch(Trunk.name())
                .build();

        log.info("Lookup for latest {} config revision...", YAML_DIR);
        return ciClient.getLastValidConfig(request).getConfigRevision();
    }

    private FrontendOnCommitFlowLaunchApi.StartFlowResponse startFlow(Context context) {
        var configRevision = lookupConfigRevision();

        var baseRequest = FrontendOnCommitFlowLaunchApi.StartFlowRequest.newBuilder()
                .setFlowProcessId(
                        ru.yandex.ci.api.proto.Common.FlowProcessId.newBuilder()
                                .setDir(YAML_DIR)
                                .setId(getFlowId(
                                        context.getCheck().getType(),
                                        context.getCheck().isStressTest()
                                ))
                )
                .setBranch(context.getCheck().getRight().getBranch())
                .setRevision(
                        ru.yandex.ci.api.proto.Common.CommitId.newBuilder()
                                .setCommitId(context.getCheck().getRight().getRevision())
                )
                .setConfigRevision(configRevision)
                .setNotifyPullRequest(false)
                .build();

        var request = StorageApi.ExtendedStartFlowRequest.newBuilder()
                .setRequest(baseRequest)
                .setFlowVars(createFlowVars(context, context.getCheck().isStressTest()))
                .setFlowTitle("Restart of " + context.getRestartIterationId())
                .build();

        return ciClient.startFlow(request);
    }

    private String getFlowId(CheckOuterClass.CheckType type, boolean stressTest) {
        if (stressTest) {
            return switch (type) {
                case TRUNK_PRE_COMMIT -> AutocheckConstants.STRESS_TEST_PRECOMMIT_TRUNK_RECHECK_PROCESS_ID.getSubId();
                case BRANCH_PRE_COMMIT, BRANCH_POST_COMMIT, TRUNK_POST_COMMIT,
                        UNRECOGNIZED -> throw new RuntimeException(
                        "Unsupported check type %s, stressTest=true".formatted(type)
                );
            };
        }
        return switch (type) {
            case BRANCH_PRE_COMMIT -> PRECOMMIT_BRANCH_ACTION;
            case BRANCH_POST_COMMIT -> POSTCOMMIT_BRANCH_ACTION;
            case TRUNK_PRE_COMMIT -> PRECOMMIT_TRUNK_ACTION;
            case TRUNK_POST_COMMIT -> POSTCOMMIT_TRUNK_ACTION;
            case UNRECOGNIZED -> throw new RuntimeException("Unknown check type");
        };
    }

    private String createFlowVars(Context context, boolean stressTest) {
        var vars = Map.of(
                "ci_check_id", Long.toString(context.check.getId().getId()),
                "ci_iteration_type", Integer.toString(context.getRestartIterationId().getIterationType().getNumber()),
                "ci_iteration_number", Integer.toString(context.getRestartIterationId().getNumber()),
                FlowVars.IS_STRESS_TEST, stressTest
        );

        try {
            return CiJson.mapper().writeValueAsString(vars);
        } catch (JsonProcessingException e) {
            throw new RuntimeException("Unable to render object as JSON", e);
        }
    }

    private SuiteRestartEntity createSuiteRestart(
            CheckIterationEntity.Id iterationId,
            Context context,
            TestDiffImportantEntity diff,
            List<TestResultEntity> runs,
            boolean isRight
    ) {
        var run = runs.get(0);
        var task = context.getTasks().get(run.getId().getTaskId());

        Preconditions.checkNotNull(task, "Task not found " + run.getId().getTaskId());

        var suiteRestartId = new SuiteRestartEntity.Id(
                iterationId,
                diff.getId().getSuiteId(),
                diff.getId().getToolchain(),
                run.getId().getPartition(),
                isRight
        );

        return new SuiteRestartEntity(suiteRestartId, diff.getId().getPath(), task.getJobName());
    }

    private List<TestResultEntity> loadSuiteRuns(
            Context context, List<Long> suiteIds
    ) {
        return db.currentOrReadOnly(() -> db.testResults().findSuites(context.getOriginalIterationId(), suiteIds));
    }

    private List<TestDiffImportantEntity> findSuitesForRestart(CheckIterationEntity.Id iterationIdToSearch) {
        return db.scan().run(() -> db.importantTestDiffs().searchSuitesNotAllToolchains(
                iterationIdToSearch,
                SuiteSearchFilters.builder()
                        .category(StorageFrontApi.CategoryFilter.CATEGORY_CHANGED)
                        .status(StorageFrontApi.StatusFilter.STATUS_FAILED)
                        .resultTypes(
                                Set.of(
                                        Common.ResultType.RT_TEST_SUITE_SMALL,
                                        Common.ResultType.RT_TEST_SUITE_MEDIUM
                                )
                        )
                        .build(),
                maxRecheckableTargets
        ));
    }

    private Context createContext(CheckIterationEntity.Id iterationId) {
        var check = db.checks().get(iterationId.getCheckId());
        var restartIteration = db.checkIterations().get(iterationId);
        var recheckFor = Integer.parseInt(restartIteration.getAttribute(Common.StorageAttribute.SA_RECHECK_FOR));
        var originalIteration = db.checkIterations().get(iterationId.toIterationId(recheckFor));

        var tasks = db.checkTasks().getByIteration(originalIteration.getId()).stream().collect(
                Collectors.toMap(x -> x.getId().getTaskId(), Function.identity())
        );

        return new Context(check, restartIteration.getId(), originalIteration.getId(), tasks);
    }

    @Data
    @AllArgsConstructor
    private static class Context {
        CheckEntity check;
        CheckIterationEntity.Id restartIterationId;
        CheckIterationEntity.Id originalIterationId;
        Map<String, CheckTaskEntity> tasks;
    }
}
