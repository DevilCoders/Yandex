package ru.yandex.ci.storage.core.large;

import java.time.Instant;
import java.util.ArrayList;
import java.util.Collection;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.function.Consumer;
import java.util.function.Function;
import java.util.function.Supplier;
import java.util.stream.Collectors;
import java.util.stream.IntStream;
import java.util.stream.Stream;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.node.IntNode;
import com.fasterxml.jackson.databind.node.ObjectNode;
import com.fasterxml.jackson.databind.node.TextNode;
import com.google.common.base.Suppliers;
import com.google.common.primitives.UnsignedLong;
import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Data;
import lombok.Singular;
import lombok.Value;
import org.assertj.core.api.Assertions;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeEach;
import org.mockito.ArgumentCaptor;
import org.mockito.Captor;
import org.mockito.Mockito;
import org.mockito.internal.verification.AtLeast;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.test.context.ContextConfiguration;

import ru.yandex.ci.api.internal.frontend.flow.oncommit.FrontendOnCommitFlowLaunchApi;
import ru.yandex.ci.api.proto.Common.FlowProcessId;
import ru.yandex.ci.api.storage.StorageApi;
import ru.yandex.ci.client.ci.CiClientImpl;
import ru.yandex.ci.core.config.VirtualCiProcessId.VirtualType;
import ru.yandex.ci.core.pr.ArcanumCheckType;
import ru.yandex.ci.core.proto.ProtobufSerialization;
import ru.yandex.ci.storage.api.StorageApi.GetLargeTaskResponse;
import ru.yandex.ci.storage.api.StorageApi.LargeTestJob;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.CheckOuterClass;
import ru.yandex.ci.storage.core.CheckTaskOuterClass;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.StorageYdbTestBase;
import ru.yandex.ci.storage.core.cache.StorageCoreCache;
import ru.yandex.ci.storage.core.check.ArcanumCheckStatus;
import ru.yandex.ci.storage.core.check.tasks.ArcanumCheckStatusReporterTask;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check.DistbuildPriority;
import ru.yandex.ci.storage.core.db.model.check.Zipatch;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckTaskStatus;
import ru.yandex.ci.storage.core.db.model.check_merge_requirements.CheckMergeRequirementsEntity;
import ru.yandex.ci.storage.core.db.model.check_task.CheckTaskEntity;
import ru.yandex.ci.storage.core.db.model.check_task.LargeTaskEntity;
import ru.yandex.ci.storage.core.db.model.chunk.ChunkEntity;
import ru.yandex.ci.storage.core.db.model.chunk_aggregate.ChunkAggregateEntity;
import ru.yandex.ci.storage.core.db.model.common.StorageRevision;
import ru.yandex.ci.storage.core.db.model.test.TestEntity;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffByHashEntity;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffEntity;
import ru.yandex.ci.storage.core.logbroker.event_producer.StorageEventsProducer;
import ru.yandex.ci.storage.core.proto.CheckProtoMappers;
import ru.yandex.ci.storage.tms.spring.LargeTasksConfig;
import ru.yandex.ci.storage.tms.spring.LargeTasksTestConfig;
import ru.yandex.ci.test.TestUtils;
import ru.yandex.ci.util.CiJson;
import ru.yandex.commune.bazinga.BazingaTaskManager;
import ru.yandex.commune.bazinga.impl.OnetimeJob;
import ru.yandex.commune.bazinga.impl.OnetimeUtils;
import ru.yandex.commune.bazinga.impl.TaskId;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;
import ru.yandex.misc.db.q.SqlLimits;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;


@ContextConfiguration(classes = {
        LargeTasksConfig.class,
        LargeTasksTestConfig.class
})
abstract class AbstractLargeAutostartServiceTest extends StorageYdbTestBase {

    protected final CheckEntity sampleCheckWithRev = sampleCheck.toBuilder()
            .left(new StorageRevision("trunk", "left", 200, Instant.EPOCH))
            .right(new StorageRevision("pr:12345", "right", 0, Instant.EPOCH))
            .distbuildPriority(new DistbuildPriority(0, 123))
            .zipatch(new Zipatch("zipatch:https://zipatch", 400))
            .type(CheckOuterClass.CheckType.BRANCH_PRE_COMMIT)
            .reportStatusToArcanum(true)
            .build();

    @MockBean
    CiClientImpl ciClient;

    @Autowired
    BazingaTaskManager bazingaTaskManager;

    @Autowired
    LargeFlowTask largeFlowTask;

    @Autowired
    LargeLogbrokerTask largeLogbrokerTask;

    @Autowired
    MarkDiscoveredCommitTask markDiscoveredCommitTask;

    @Autowired
    AutocheckTasksFactory autocheckTasksFactory;

    @Autowired
    StorageCoreCache<?> storageCache;

    @MockBean
    StorageEventsProducer storageEventsProducer;

    @Captor
    ArgumentCaptor<CheckOuterClass.Check> iterCheckCap;

    @Captor
    ArgumentCaptor<CheckIteration.Iteration> iterIterationCap;

    @Captor
    ArgumentCaptor<CheckOuterClass.Check> taskCheckCap;

    @Captor
    ArgumentCaptor<CheckIteration.Iteration> taskIterationCap;

    @Captor
    ArgumentCaptor<List<CheckTaskOuterClass.CheckTask>> taskTasksCap;

    @Captor
    ArgumentCaptor<StorageApi.ExtendedStartFlowRequest> extendedStartFlowRequestCap;

    @BeforeEach
    void resetMocks() {
        this.storageCache.modify(StorageCoreCache.Modifiable::invalidateAll);
        Mockito.reset(ciClient);
        Mockito.reset(storageEventsProducer);

    }

    @AfterEach
    void checkNoInvocation() {
        Mockito.verifyNoMoreInteractions(ciClient);
        Mockito.verifyNoMoreInteractions(storageEventsProducer);
    }


    @Value
    @Builder
    static class LargeTestSettings {
        @Nonnull
        CheckEntity check;

        @Nonnull
        TestStatuses testStatuses;

        @Nonnull
        Supplier<CheckIterationEntity> saveIteration;

        @Nonnull
        Consumer<CheckIterationEntity> checkIteration;

        @Singular
        List<LargeStart> expectTasks;

        @Nonnull
        StorageApi.DelegatedConfig largeTestsDelegatedConfig;

        @Nonnull
        StorageApi.DelegatedConfig nativeBuildsDelegatedConfig;

        Supplier<JsonNode> largeTestsDelegatedConfigNode = Suppliers.memoize(() ->
                asJsonOrNull(getLargeTestsDelegatedConfig()));

        Supplier<JsonNode> nativeBuildsDelegatedConfigNode = Suppliers.memoize(() ->
                asJsonOrNull(getNativeBuildsDelegatedConfig()));

        @Nonnull
        Consumer<List<TestDiffEntity>> executor;

        boolean reportEmptyTests;

        String startedBy;

        Boolean launchable;

        boolean manual;

        @Nullable
        private static JsonNode asJsonOrNull(StorageApi.DelegatedConfig delegatedConfig) {
            return CheckProtoMappers.hasDelegatedConfig(delegatedConfig)
                    ? TestUtils.readJson(CheckProtoMappers.toDelegatedConfig(delegatedConfig))
                    : null;
        }

        public static class Builder {
            {
                largeTestsDelegatedConfig = StorageApi.DelegatedConfig.getDefaultInstance();
                nativeBuildsDelegatedConfig = StorageApi.DelegatedConfig.getDefaultInstance();
                launchable = true;
            }
        }
    }

    @AllArgsConstructor
    class LargeTest {
        @Nonnull
        private final LargeTestSettings settings;

        Set<Common.CheckTaskType> test() {
            var check = settings.check;
            var iteration = settings.saveIteration.get();

            var aggregateId = new ChunkAggregateEntity.Id(
                    iteration.getId(), ChunkEntity.Id.of(Common.ChunkType.CT_LARGE_TEST, 1));

            var largeTestsDiffs = IntStream.of(1, 2, 3)
                    .mapToObj(suffix -> {
                        var suiteId = UnsignedLong.valueOf(Long.MAX_VALUE)
                                .plus(UnsignedLong.valueOf(suffix))
                                .longValue();
                        var test = TestEntity.Id.of(suiteId, "chain-" + suffix);
                        var requirements = switch (suffix) {
                            case 1 -> "{\"sb_vault\": \"token-1\", \"ram\": 32}";
                            case 2 -> "";
                            case 3 -> null;
                            default -> throw new IllegalStateException("Unsupported value: " + suffix);
                        };
                        var testDiff = TestDiffByHashEntity.builder()
                                .id(TestDiffByHashEntity.Id.of(aggregateId, test))
                                .name("java")
                                .path("a/b/c")
                                .isLast(true)
                                .oldSuiteId("02f327ca347f486b087713a01b51e115")
                                .resultType(Common.ResultType.RT_TEST_SUITE_LARGE)
                                .left(settings.testStatuses.leftStatus)
                                .right(settings.testStatuses.rightStatus)
                                .tags(Set.of("t"))
                                .requirements(requirements)
                                .isLaunchable(settings.launchable)
                                .build();
                        return new TestDiffEntity(testDiff);
                    })
                    .toList();

            var nativeBuildsDiffs = Stream.of("a/b", "b/c", "c/d")
                    .map(path -> {
                        var suiteId = UnsignedLong.valueOf(Long.MAX_VALUE)
                                .plus(UnsignedLong.valueOf(1000000))
                                .plus(UnsignedLong.valueOf(path.hashCode()))
                                .longValue();
                        var test = TestEntity.Id.of(suiteId, "build-chain-1");
                        var testDiff = TestDiffByHashEntity.builder()
                                .id(TestDiffByHashEntity.Id.of(aggregateId, test))
                                .name("java")
                                .path(path)
                                .isLast(true)
                                .oldSuiteId("02f327ca347f486b087713a01b51e115")
                                .resultType(Common.ResultType.RT_BUILD)
                                .left(settings.testStatuses.leftBuildStatus)
                                .right(settings.testStatuses.rightBuildStatus)
                                .tags(Set.of("t"))
                                .build();
                        return new TestDiffEntity(testDiff);
                    })
                    .toList();

            db.currentOrTx(() -> {
                db.checks().save(check);
                db.testDiffs().save(largeTestsDiffs);
                db.testDiffs().save(nativeBuildsDiffs);
                db.checkIterations().save(iteration);
            });

            var expectConfigRequest = StorageApi.GetLastValidConfigRequest.newBuilder()
                    .setDir("autocheck/large-tests")
                    .setBranch("trunk")
                    .build();

            var configResponse = StorageApi.GetLastValidConfigResponse.newBuilder()
                    .setConfigRevision(ru.yandex.ci.api.proto.Common.OrderedArcRevision.newBuilder()
                            .setBranch("trunk")
                            .setHash("latest"))
                    .build();

            Mockito.when(ciClient.getLastValidConfig(Mockito.eq(expectConfigRequest)))
                    .thenReturn(configResponse);

            // Execute test
            settings.executor.accept(largeTestsDiffs);

            // Run all bazinga tasks
            runLogbrokerTasks();

            var updatedIteration = db.currentOrReadOnly(() -> db.checkIterations().get(iteration.getId()));
            settings.checkIteration.accept(updatedIteration);

            if (!settings.expectTasks.isEmpty()) {
                Mockito.verify(ciClient, new AtLeast(1))
                        .getLastValidConfig(eq(StorageApi.GetLastValidConfigRequest.newBuilder()
                                .setDir("autocheck/large-tests")
                                .setBranch("trunk")
                                .build())
                        );
            }

            var currentCheckTasksTasks = currentHeavyCheckTasks();
            var currentLargeTasks = currentLargeTasks();
            var currentIterations = currentHeavyIterations(currentCheckTasksTasks);
            var expectTasks = splitExpectTasks();

            assertThat(expectTasks.keySet())
                    .isEqualTo(currentIterations.keySet());

            for (var entry : currentIterations.entrySet()) {
                assertThat(entry.getValue().getNumberOfTasks())
                        .isEqualTo(expectTasks.get(entry.getKey()).size());
            }

            this.verifyMergeRequirements(
                    iteration, currentIterations.values(), currentCheckTasksTasks.values()
            );

            Mockito.verify(storageEventsProducer, Mockito.times(expectTasks.size()))
                    .onIterationRegistered(iterCheckCap.capture(), iterIterationCap.capture());
            Mockito.verify(storageEventsProducer, Mockito.times(expectTasks.size()))
                    .onTasksRegistered(taskCheckCap.capture(), taskIterationCap.capture(), taskTasksCap.capture());
            for (var type : expectTasks.keySet()) {
                this.verifyStorageTasks(
                        currentIterations.get(type),
                        currentCheckTasksTasks.get(type),
                        expectTasks.get(type)
                );
            }
            for (var type : expectTasks.keySet()) {
                this.verifyLargeTaskEntities(
                        check.getType(),
                        type,
                        currentIterations.get(type),
                        currentCheckTasksTasks.get(type),
                        currentLargeTasks.get(type),
                        expectTasks.get(type)
                );
            }

            verifyDiscoveredCommitsAreMarked(check.getType(), settings.manual);

            return expectTasks.keySet();
        }

        private void runLogbrokerTasks() {
            var bazingaJobs = bazingaTaskManager.getActiveJobs(TaskId.from(LargeLogbrokerTask.class), SqlLimits.all());
            var iterations = bazingaJobs.stream()
                    .map(OnetimeJob::getParameters)
                    .map(params -> (BazingaIterationId) OnetimeUtils.parseParameters(largeLogbrokerTask, params))
                    .toList();
            for (var iterationId : iterations) {
                largeLogbrokerTask.execute(iterationId, Mockito.mock(ExecutionContext.class));
            }
        }

        private void runMarkDiscoveredCommitTasks() {
            var bazingaJobs = bazingaTaskManager.getActiveJobs(TaskId.from(MarkDiscoveredCommitTask.class),
                    SqlLimits.all());
            var checks = bazingaJobs.stream()
                    .map(OnetimeJob::getParameters)
                    .map(params -> (BazingaCheckId) OnetimeUtils.parseParameters(markDiscoveredCommitTask, params))
                    .toList();
            for (var check : checks) {
                markDiscoveredCommitTask.execute(check, Mockito.mock(ExecutionContext.class));
            }
        }

        private void verifyMergeRequirements(
                CheckIterationEntity iteration,
                Collection<CheckIterationEntity> iterations,
                Collection<List<CheckTaskEntity>> tasks
        ) {
            var largeRequirementId = new CheckMergeRequirementsEntity.Id(
                    settings.check.getId(), ArcanumCheckType.CI_LARGE_TESTS
            );

            var nativeRequirementId = new CheckMergeRequirementsEntity.Id(
                    settings.check.getId(), ArcanumCheckType.CI_BUILD_NATIVE
            );

            var largeRequirement = db.currentOrReadOnly(() -> db.checkMergeRequirements().find(largeRequirementId));
            var nativeRequirement = db.currentOrReadOnly(() -> db.checkMergeRequirements().find(nativeRequirementId));
            var requirementsJobs = bazingaTaskManager.getActiveJobs(TaskId.from(ArcanumCheckStatusReporterTask.class),
                    SqlLimits.all());

            var reportToArcanum = LargeStartService.isPrecommit(settings.check.getType());
            if (settings.expectTasks.isEmpty()) {
                Assertions.assertThat(iterations).isEmpty();
                Assertions.assertThat(tasks).isEmpty();

                var largeStatus = iteration.getCheckTaskStatuses().getOrDefault(
                        Common.CheckTaskType.CTT_LARGE_TEST, CheckTaskStatus.NOT_REQUIRED
                );

                var nativeStatus = iteration.getCheckTaskStatuses().getOrDefault(
                        Common.CheckTaskType.CTT_NATIVE_BUILD, CheckTaskStatus.NOT_REQUIRED
                );

                if (settings.reportEmptyTests &&
                        reportToArcanum &&
                        (largeStatus == CheckTaskStatus.SCHEDULED || nativeStatus == CheckTaskStatus.SCHEDULED)
                ) {
                    if (largeStatus == CheckTaskStatus.SCHEDULED) {
                        assertThat(largeRequirement).isPresent();
                        assertThat(largeRequirement.orElseThrow())
                                .isEqualTo(CheckMergeRequirementsEntity.builder()
                                        .id(largeRequirementId)
                                        .updatedAt(largeRequirement.get().getUpdatedAt())
                                        .reportedAt(null)
                                        .value(ArcanumCheckStatus.skipped("No Tests matched"))
                                        .build());
                    } else {
                        assertThat(nativeRequirement).isPresent();
                        assertThat(nativeRequirement.orElseThrow())
                                .isEqualTo(CheckMergeRequirementsEntity.builder()
                                        .id(nativeRequirementId)
                                        .updatedAt(nativeRequirement.get().getUpdatedAt())
                                        .reportedAt(null)
                                        .value(ArcanumCheckStatus.skipped("No Tests matched"))
                                        .build());
                    }
                    assertThat(requirementsJobs).hasSize(1);
                } else {
                    assertThat(largeRequirement).isEmpty();
                    assertThat(nativeRequirement).isEmpty();
                    assertThat(requirementsJobs).isEmpty();
                }
            } else {
                if (reportToArcanum) {
                    assertThat(largeRequirement.isPresent() || nativeRequirement.isPresent()).isTrue();
                    if (largeRequirement.isPresent()) {
                        assertThat(largeRequirement.orElseThrow())
                                .isEqualTo(CheckMergeRequirementsEntity.builder()
                                        .id(largeRequirementId)
                                        .updatedAt(largeRequirement.get().getUpdatedAt())
                                        .reportedAt(null)
                                        .value(ArcanumCheckStatus.pending())
                                        .build());
                    } else {
                        assertThat(nativeRequirement.orElseThrow())
                                .isEqualTo(CheckMergeRequirementsEntity.builder()
                                        .id(nativeRequirementId)
                                        .updatedAt(nativeRequirement.get().getUpdatedAt())
                                        .reportedAt(null)
                                        .value(ArcanumCheckStatus.pending())
                                        .build());
                    }

                    assertThat(requirementsJobs).hasSize(
                            (largeRequirement.isPresent() ? 1 : 0) + (nativeRequirement.isPresent() ? 1 : 0)
                    );
                } else {
                    assertThat(largeRequirement).isEmpty();
                    assertThat(nativeRequirement).isEmpty();
                    assertThat(requirementsJobs).isEmpty();
                }
            }
        }

        private void verifyStorageTasks(
                CheckIterationEntity iteration,
                List<CheckTaskEntity> checkTasks,
                List<LargeStart> expectTasks
        ) {
            var checkTaskIds = checkTasks.stream()
                    .map(CheckTaskEntity::getId)
                    .collect(Collectors.toSet());

            var index = new Index();
            var expectTaskIds = expectTasks.stream()
                    .map(e -> new CheckTaskEntity.Id(iteration.getId(), String.valueOf(index.inc())))
                    .collect(Collectors.toSet());
            Assertions.assertThat(checkTaskIds)
                    .isEqualTo(expectTaskIds);

            // Expected tasks - not configured
            assertThat(iteration.getExpectedTasks())
                    .isEmpty();

            var protoCheckId = settings.check.getId().getId().toString();
            var protoIterationId = CheckProtoMappers.toProtoIterationId(iteration.getId());

            assertThat(iterCheckCap.getAllValues().stream().map(CheckOuterClass.Check::getId))
                    .contains(protoCheckId);
            assertThat(iterIterationCap.getAllValues().stream().map(CheckIteration.Iteration::getId))
                    .contains(protoIterationId);

            assertThat(taskCheckCap.getAllValues().stream().map(CheckOuterClass.Check::getId))
                    .contains(protoCheckId);
            assertThat(taskIterationCap.getAllValues().stream().map(CheckIteration.Iteration::getId))
                    .contains(protoIterationId);
            assertThat(taskTasksCap.getAllValues().stream()
                    .flatMap(Collection::stream)
                    .map(CheckTaskOuterClass.CheckTask::getId))
                    .containsAll(expectTaskIds.stream()
                            .map(CheckProtoMappers::toProtoCheckTaskId)
                            .collect(Collectors.toSet()));
        }

        private void verifyNoDiscoveredCommitsAreMarked() {
            runMarkDiscoveredCommitTasks();
            Mockito.verify(ciClient, Mockito.never())
                    .markDiscoveredCommit(any(StorageApi.MarkDiscoveredCommitRequest.class));
        }

        private void verifyDiscoveredCommitsAreMarked(CheckOuterClass.CheckType checkType, boolean isManual) {
            if (checkType == CheckOuterClass.CheckType.TRUNK_POST_COMMIT && !isManual) {
                runMarkDiscoveredCommitTasks();
                Mockito.verify(ciClient)
                        .markDiscoveredCommit(eq(StorageApi.MarkDiscoveredCommitRequest.newBuilder()
                                .setRevision(ru.yandex.ci.api.proto.Common.CommitId.newBuilder()
                                        .setCommitId(settings.check.getRight().getRevision())
                                        .build())
                                .build()));
            } else {
                verifyNoDiscoveredCommitsAreMarked();
            }
        }

        private void verifyTaskProcessesMatched(
                List<CheckTaskEntity> checkTasks,
                List<LargeTaskEntity> largeTasks
        ) {
            var checkTaskMap = checkTasks.stream()
                    .collect(Collectors.toMap(CheckTaskEntity::getId, Function.identity()));

            for (var largeTask : largeTasks) {
                var expectCiProcessId = largeTask.toCiProcessId().asString();
                checkLargeTask(expectCiProcessId, "left", largeTask.toLeftTaskId(), checkTaskMap);
                checkLargeTask(expectCiProcessId, "right", largeTask.toRightTaskId(), checkTaskMap);
            }
        }

        private void checkLargeTask(
                String expectCiProcessId,
                String side,
                @Nullable CheckTaskEntity.Id checkTaskId,
                Map<CheckTaskEntity.Id, CheckTaskEntity> checkTaskMap
        ) {
            if (checkTaskId == null) {
                return;
            }
            var checkTask = checkTaskMap.get(checkTaskId);
            assertThat(checkTask)
                    .describedAs("Reference to %s checkTask must exists", side)
                    .isNotNull();
            assertThat(expectCiProcessId)
                    .describedAs("Large task processId must be the same as %s processId", side)
                    .isEqualTo(checkTask.getJobName());

        }

        private void verifyLargeTaskEntities(
                CheckOuterClass.CheckType checkType,
                Common.CheckTaskType checkTaskType,
                CheckIterationEntity iteration,
                List<CheckTaskEntity> checkTasks,
                List<LargeTaskEntity> largeTasks,
                List<LargeStart> expectTasks
        ) {
            verifyTaskProcessesMatched(checkTasks, largeTasks);

            var expectMap = new LinkedHashMap<String, LargeTaskDescription>(expectTasks.size() / 2 + 1);
            var testIndex = new Index();
            var taskIndex = new Index();
            for (var task : expectTasks) {
                var desc = expectMap.computeIfAbsent(task.getLargeTaskJsonResource(),
                        res -> new LargeTaskDescription(
                                new LargeTaskEntity.Id(iteration.getId(), checkTaskType, testIndex.inc()))
                );
                if (task.right) {
                    desc.setRightTaskId(taskIndex.inc());
                    desc.setRightStart(task);
                } else {
                    desc.setLeftTaskId(taskIndex.inc());
                    desc.setLeftStart(task);
                }
            }

            var expectLargeTasks = expectMap.entrySet().stream()
                    .map(e -> {
                        var node = TestUtils.readJson(e.getKey());
                        return updateLargeTaskEntity(checkTaskType, node, e.getValue());
                    })
                    .toList();


            var actualLargeTasks = largeTasks.stream()
                    .map(TestUtils::readJson)
                    .toList();

            assertThat(actualLargeTasks)
                    .isEqualTo(expectLargeTasks);


            // All registered large tasks must be scheduled for execution
            var expectBazingaLargeTasks = expectMap.values().stream()
                    .map(LargeTaskDescription::getLargeTaskId)
                    .toList();

            var bazingaJobs = bazingaTaskManager.getActiveJobs(TaskId.from(LargeFlowTask.class), SqlLimits.all());
            var bazingaParams = bazingaJobs.stream()
                    .map(OnetimeJob::getParameters)
                    .sorted()
                    .map(params -> (LargeFlowTask.Params) OnetimeUtils.parseParameters(largeFlowTask, params))
                    .filter(params -> params.getType() == checkTaskType)
                    .toList();

            var actualBazingaLargeTasks = bazingaParams.stream()
                    .map(LargeFlowTask.Params::getLargeTaskId)
                    .filter(id -> id.getCheckTaskType() == checkTaskType)
                    .toList();

            assertThat(actualBazingaLargeTasks)
                    .isEqualTo(expectBazingaLargeTasks);

            // Execute tasks and check parameters we use to launch flow
            executeLargeFlowTasks(checkType, checkTaskType, bazingaParams, List.copyOf(expectMap.values()), largeTasks);
        }

        private JsonNode getDelegatedConfig(Common.CheckTaskType checkTaskType) {
            return switch (checkTaskType) {
                case CTT_LARGE_TEST -> settings.largeTestsDelegatedConfigNode.get();
                case CTT_NATIVE_BUILD -> settings.nativeBuildsDelegatedConfigNode.get();
                default -> throw new UnsupportedOperationException("Unsupported check task type: " + checkTaskType);
            };
        }

        private ObjectNode updateLargeTaskEntity(
                Common.CheckTaskType checkTaskType,
                ObjectNode node,
                LargeTaskDescription desc
        ) {
            var delegatedConfig = getDelegatedConfig(checkTaskType);

            var id = (ObjectNode) node.with("id");
            id.set("index", new IntNode(desc.largeTaskId.getIndex()));

            var iteration = (ObjectNode) id.with("iterationId");
            iteration.set("number", new IntNode(desc.largeTaskId.getIterationId().getNumber()));

            if (desc.leftTaskId != null) {
                node.set("leftTaskId", new TextNode(desc.leftTaskId.toString()));
            } else {
                node.remove("leftTaskId");
                node.remove("leftLargeTestInfo");
            }
            if (desc.rightTaskId != null) {
                node.set("rightTaskId", new TextNode(desc.rightTaskId.toString()));
            } else {
                node.remove("rightTaskId");
                node.remove("rightLargeTestInfo");
            }
            if (settings.startedBy != null) {
                node.set("startedBy", new TextNode(settings.startedBy));
            } else {
                node.remove("startedBy");
            }
            if (delegatedConfig != null) {
                node.set("delegatedConfig", delegatedConfig);
            } else {
                node.remove("delegatedConfig");
            }
            return node;
        }

        private void executeLargeFlowTasks(
                CheckOuterClass.CheckType checkType,
                Common.CheckTaskType checkTaskType,
                List<LargeFlowTask.Params> actualParams,
                List<LargeTaskDescription> expectTasks,
                List<LargeTaskEntity> largeTasks) {

            assertThat(actualParams)
                    .hasSize(expectTasks.size());

            verifyNoDiscoveredCommitsAreMarked();

            for (int i = 0; i < actualParams.size(); i++) {
                var actualParam = actualParams.get(i);
                var expectTask = expectTasks.get(i);
                var largeTask = largeTasks.get(i);

                // Catch flow request
                Mockito.when(ciClient.startFlow(any(StorageApi.ExtendedStartFlowRequest.class)))
                        .thenReturn(FrontendOnCommitFlowLaunchApi.StartFlowResponse.newBuilder()
                                .setLaunch(FrontendOnCommitFlowLaunchApi.FlowLaunch.newBuilder().setNumber(i))
                                .build());
                largeFlowTask.execute(actualParam, Mockito.mock(ExecutionContext.class));

                Mockito.verify(ciClient, Mockito.atLeastOnce())
                        .startFlow(extendedStartFlowRequestCap.capture());
                var actualStartFlowRequest = extendedStartFlowRequestCap.getValue();

                var expectStartFlowRequest = TestUtils.parseProtoText("large/chain-flow.pb",
                        StorageApi.ExtendedStartFlowRequest.class);

                var expectFlowRequest = updateExtendedFlowRequest(
                        checkType,
                        checkTaskType,
                        expectStartFlowRequest,
                        largeTask
                );
                assertThat(actualStartFlowRequest)
                        .isEqualTo(expectFlowRequest);

                // Launch number was updated
                var currentLargeTask = db.currentOrReadOnly(() -> db.largeTasks().get(expectTask.getLargeTaskId()));
                assertThat(currentLargeTask.getLaunchNumber())
                        .isEqualTo(i);

                // Check response from LargeTasksSearch

                var actualTaskResponse = autocheckTasksFactory.loadLargeTask(
                        CheckProtoMappers.toProtoLargeTaskRequest(expectTask.getLargeTaskId()));

                var expectTaskResponse = GetLargeTaskResponse.newBuilder();
                addTaskResponse(checkType, expectTaskResponse, expectTask, false);
                addTaskResponse(checkType, expectTaskResponse, expectTask, true);

                assertThat(actualTaskResponse)
                        .isEqualTo(expectTaskResponse.build());
            }
        }

        private void addTaskResponse(
                CheckOuterClass.CheckType checkType,
                GetLargeTaskResponse.Builder response,
                LargeTaskDescription description,
                boolean right
        ) {
            var start = right ? description.rightStart : description.leftStart;
            if (start == null) {
                return; // ---
            }
            var proto = TestUtils.parseProtoText(
                    start.getLargeTaskResponseProtoResource(),
                    LargeTestJob.class
            ).toBuilder();

            var largeTaskId = description.getLargeTaskId();
            proto.getIdBuilder().getIterationIdBuilder()
                    .setNumber(largeTaskId.getIterationId().getNumber());
            var taskId = right ? description.rightTaskId : description.leftTaskId;
            proto.getIdBuilder().setTaskId(String.valueOf(taskId));

            proto.setCheckType(settings.check.getType());
            if (settings.startedBy != null) {
                proto.setStartedBy(settings.startedBy);
            }
            proto.setPrecommit(LargeStartService.isPrecommit(checkType));
            response.addJobs(proto);
        }

        private StorageApi.DelegatedConfig getProtoDelegatedConfig(Common.CheckTaskType checkTaskType) {
            return switch (checkTaskType) {
                case CTT_LARGE_TEST -> settings.largeTestsDelegatedConfig;
                case CTT_NATIVE_BUILD -> settings.nativeBuildsDelegatedConfig;
                default -> throw new UnsupportedOperationException("Unsupported check task type: " + checkTaskType);
            };
        }

        private StorageApi.ExtendedStartFlowRequest updateExtendedFlowRequest(
                CheckOuterClass.CheckType checkType,
                Common.CheckTaskType checkTaskType,
                StorageApi.ExtendedStartFlowRequest request,
                LargeTaskEntity largeTask
        ) {

            var builder = request.toBuilder();
            var testInfo = largeTask.getLeftLargeTestInfo();

            String target;
            VirtualType virtualType;
            switch (checkTaskType) {
                case CTT_LARGE_TEST -> {
                    target = largeTask.getTarget();
                    virtualType = VirtualType.VIRTUAL_LARGE_TEST;
                }
                case CTT_NATIVE_BUILD -> {
                    target = largeTask.getNativeTarget();
                    virtualType = VirtualType.VIRTUAL_NATIVE_BUILD;
                }
                default -> throw new IllegalStateException("Unsupported check task type: " + checkTaskType);
            }
            builder.getRequestBuilder().setFlowProcessId(
                    FlowProcessId.newBuilder()
                            .setDir(virtualType.getPrefix() + target)
                            .setId(testInfo.getToolchain() + "@" + testInfo.getSuiteName())
            );

            var delegatedConfig = getProtoDelegatedConfig(checkTaskType);
            if (CheckProtoMappers.hasDelegatedConfig(delegatedConfig)) {
                builder.setDelegatedConfig(delegatedConfig);
            }

            var id = CheckProtoMappers.toProtoLargeTaskRequest(largeTask.getId());
            var flowVars = CiJson.mapper().createObjectNode();
            flowVars.set("request", ProtobufSerialization.serializeToJson(id));

            var testId = CheckProtoMappers.toProtoTestId(largeTask);
            flowVars.set("testInfo", ProtobufSerialization.serializeToJson(testId));

            builder.setFlowVars(flowVars.toString());
            if (checkType == CheckOuterClass.CheckType.TRUNK_POST_COMMIT) {
                builder.setPostponed(true);
            }

            return builder.build();
        }

        private Map<Common.CheckTaskType, CheckIterationEntity> currentHeavyIterations(
                Map<Common.CheckTaskType, List<CheckTaskEntity>> tasks
        ) {
            var result = new LinkedHashMap<Common.CheckTaskType, CheckIterationEntity>();

            var iterationIds = tasks.values().stream()
                    .flatMap(Collection::stream)
                    .map(CheckTaskEntity::getId)
                    .map(CheckTaskEntity.Id::getIterationId)
                    .collect(Collectors.toSet());
            var iterations = db.currentOrReadOnly(() -> db.checkIterations().find(iterationIds)).stream()
                    .collect(Collectors.toMap(CheckIterationEntity::getId, Function.identity()));

            for (var task : tasks.entrySet()) {
                var checkTaskType = task.getKey();
                var checkIteration = task.getValue().stream()
                        .map(CheckTaskEntity::getId)
                        .map(CheckTaskEntity.Id::getIterationId)
                        .map(iterations::get)
                        .findAny()
                        .orElseThrow(() -> new RuntimeException("Unable to find iteration for " + checkTaskType));
                assertThat(checkIteration.getTasksType()).isEqualTo(checkTaskType);
                result.putIfAbsent(checkTaskType, checkIteration);
            }
            return result;
        }

        private Map<Common.CheckTaskType, List<CheckTaskEntity>> currentHeavyCheckTasks() {
            var result = new LinkedHashMap<Common.CheckTaskType, List<CheckTaskEntity>>();
            for (var task : db.currentOrReadOnly(() -> db.checkTasks().findAll())) {
                if (task.getId().getIterationId().getIterationType() == CheckIteration.IterationType.HEAVY) {
                    result.computeIfAbsent(task.getType(), type -> new ArrayList<>()).add(task);
                }
            }
            return result;
        }

        private Map<Common.CheckTaskType, List<LargeTaskEntity>> currentLargeTasks() {
            var result = new LinkedHashMap<Common.CheckTaskType, List<LargeTaskEntity>>();
            for (var task : db.currentOrReadOnly(() -> db.largeTasks().findAll())) {
                result.computeIfAbsent(task.getId().getCheckTaskType(), type -> new ArrayList<>()).add(task);
            }
            return result;
        }

        private Map<Common.CheckTaskType, List<LargeStart>> splitExpectTasks() {
            var result = new LinkedHashMap<Common.CheckTaskType, List<LargeStart>>();
            for (var start : settings.expectTasks) {
                result.computeIfAbsent(start.getCheckTaskType(), type -> new ArrayList<>()).add(start);
            }
            return result;
        }

    }


    @Value
    @AllArgsConstructor
    static class LargeStart {
        @Nonnull
        String largeTaskJsonResource;

        @Nonnull
        String largeTaskResponseProtoResource;

        @Nonnull
        Common.CheckTaskType checkTaskType;

        boolean right;

        static LargeStart largeTest(String toolchain, boolean right) {
            var name = toolchain + "-" + (right ? "right" : "left");
            return new LargeStart(
                    "large/%s.json".formatted(toolchain),
                    "large/%s.pb".formatted(name),
                    Common.CheckTaskType.CTT_LARGE_TEST,
                    right
            );
        }

        static LargeStart nativeBuild(String prefix, boolean right) {
            var name = prefix + "_" + (right ? "right" : "left");
            return new LargeStart(
                    "native-build/%s.json".formatted(prefix),
                    "native-build/%s.pb".formatted(name),
                    Common.CheckTaskType.CTT_NATIVE_BUILD,
                    right
            );
        }
    }

    @Value
    static class TestStatuses {
        @Nonnull
        Common.TestStatus leftStatus;
        @Nonnull
        Common.TestStatus rightStatus;
        @Nonnull
        Common.TestStatus leftBuildStatus;
        @Nonnull
        Common.TestStatus rightBuildStatus;

        static TestStatuses largeTests(Common.TestStatus leftStatus, Common.TestStatus rightStatus) {
            return new TestStatuses(leftStatus, rightStatus,
                    Common.TestStatus.TS_NONE, Common.TestStatus.TS_NONE);
        }

        static TestStatuses nativeBuilds(Common.TestStatus leftBuildStatus, Common.TestStatus rightBuildStatus) {
            return new TestStatuses(Common.TestStatus.TS_NONE, Common.TestStatus.TS_NONE,
                    leftBuildStatus, rightBuildStatus);
        }

        static TestStatuses all(Common.TestStatus leftStatus, Common.TestStatus rightStatus,
                                Common.TestStatus leftBuildStatus, Common.TestStatus rightBuildStatus) {
            return new TestStatuses(leftStatus, rightStatus,
                    leftBuildStatus, rightBuildStatus);
        }
    }

    @Data
    static class LargeTaskDescription {
        final LargeTaskEntity.Id largeTaskId;
        LargeStart leftStart;
        LargeStart rightStart;
        Integer leftTaskId;
        Integer rightTaskId;
    }

    static class Index {
        private int counter;

        int inc() {
            return counter++;
        }
    }
}
