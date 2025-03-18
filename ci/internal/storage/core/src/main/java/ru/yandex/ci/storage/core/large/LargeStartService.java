package ru.yandex.ci.storage.core.large;

import java.nio.charset.StandardCharsets;
import java.time.Instant;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.function.Function;
import java.util.function.Supplier;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.JsonNode;
import com.google.common.base.Preconditions;
import com.google.common.base.Suppliers;
import com.google.common.hash.Hashing;
import com.google.common.primitives.UnsignedLong;
import lombok.AllArgsConstructor;
import lombok.RequiredArgsConstructor;
import lombok.Value;
import lombok.extern.slf4j.Slf4j;
import org.apache.commons.lang3.StringUtils;
import org.apache.logging.log4j.util.Strings;

import ru.yandex.ci.api.proto.Common;
import ru.yandex.ci.api.storage.StorageApi;
import ru.yandex.ci.client.ci.CiClient;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.config.VirtualCiProcessId;
import ru.yandex.ci.core.pr.ArcanumCheckType;
import ru.yandex.ci.core.proto.CiCoreProtoMappers;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.CheckOuterClass;
import ru.yandex.ci.storage.core.Common.CheckTaskType;
import ru.yandex.ci.storage.core.Common.TestStatus;
import ru.yandex.ci.storage.core.cache.StorageCoreCache;
import ru.yandex.ci.storage.core.check.ArcanumCheckStatus;
import ru.yandex.ci.storage.core.check.CheckService;
import ru.yandex.ci.storage.core.check.CheckTaskTypeUtils;
import ru.yandex.ci.storage.core.check.CreateIterationParams;
import ru.yandex.ci.storage.core.check.RequirementsService;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckTaskStatus;
import ru.yandex.ci.storage.core.db.model.check_task.CheckTaskEntity;
import ru.yandex.ci.storage.core.db.model.check_task.CheckTaskTable;
import ru.yandex.ci.storage.core.db.model.check_task.LargeTaskEntity;
import ru.yandex.ci.storage.core.db.model.check_task.LargeTestInfo;
import ru.yandex.ci.storage.core.db.model.check_task.NativeSpecification;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffEntity;
import ru.yandex.ci.storage.core.logbroker.event_producer.StorageEventsProducer;
import ru.yandex.ci.storage.core.proto.CheckProtoMappers;
import ru.yandex.ci.util.CiJson;
import ru.yandex.commune.bazinga.BazingaTaskManager;

@Slf4j
@RequiredArgsConstructor
public class LargeStartService {

    @Nonnull
    private final CiStorageDb db;

    @Nonnull
    private final BazingaTaskManager bazingaTaskManager;

    @Nonnull
    private final RequirementsService requirementsService;

    @Nonnull
    private final CiClient ciClient;

    @Nonnull
    private final CheckService checkService;

    @Nonnull
    private final StorageEventsProducer storageEventsProducer;

    @Nonnull
    private final StorageCoreCache<?> storageCache;

    public void scheduleLargeTestsManual(
            CheckEntity.Id checkId,
            boolean runLargeTestsAfterDiscovery,
            String startedBy
    ) {
        storageCache.modifyWithDbTx(cache -> {
            var check = cache.checks().getFreshOrThrow(checkId);
            log.info("Check {}, runLargeTestsAfterDiscovery, old = {}, new = {}, username = {}",
                    checkId, check.getRunLargeTestsAfterDiscovery(), runLargeTestsAfterDiscovery, startedBy);

            if (check.getRunLargeTestsAfterDiscovery() == runLargeTestsAfterDiscovery) {
                updateRunLargeTestsAfterDiscovery(cache, check, runLargeTestsAfterDiscovery, startedBy);
                return; // Don't even check anything
            }

            var firstIteration = cache.iterations().getFreshOrThrow(
                    CheckIterationEntity.Id.of(checkId, CheckIteration.IterationType.FULL, 1));
            var largeTestStatus = firstIteration.getCheckTaskStatus(CheckTaskType.CTT_LARGE_TEST);
            if (runLargeTestsAfterDiscovery && !largeTestStatus.isDiscovering()) {
                log.info("Iteration {}, unable to schedule large tests, current status is {}",
                        firstIteration.getId(), largeTestStatus);

                // Maybe run large tests right now?
                throw new IllegalArgumentException("Large tests already discovered, cannot schedule execution");
            }

            check = updateRunLargeTestsAfterDiscovery(cache, check, runLargeTestsAfterDiscovery, startedBy);
            var status = runLargeTestsAfterDiscovery || largeTestStatus == CheckTaskStatus.DISCOVERING
                    ? ArcanumCheckStatus.pending()
                    : ArcanumCheckStatus.skipped();

            requirementsService.scheduleRequirement(
                    cache,
                    check.getId(),
                    ArcanumCheckType.CI_LARGE_TESTS,
                    status
            );
        });
    }

    private CheckEntity updateRunLargeTestsAfterDiscovery(
            StorageCoreCache.Modifiable cache,
            CheckEntity check,
            boolean runLargeTestsAfterDiscovery,
            String startedBy
    ) {
        check = check.withRunLargeTestsAfterDiscovery(runLargeTestsAfterDiscovery);
        check = check.withRunLargeTestsAfterDiscoveryBy(
                runLargeTestsAfterDiscovery
                        ? startedBy
                        : null
        );
        cache.checks().writeThrough(check);
        return check;
    }

    public void startLargeTestsManual(@Nonnull List<TestDiffEntity.Id> testIds, String startedBy) {
        if (testIds.isEmpty()) {
            log.info("No Large test to start");
            return;
        }

        var keys = new LinkedHashSet<>(testIds);

        var checkIds = keys.stream()
                .map(TestDiffEntity.Id::getIterationId)
                .map(CheckIterationEntity.Id::getCheckId)
                .distinct()
                .toList();

        if (checkIds.size() != 1) {
            throw new IllegalArgumentException("All tests must be from single Check, found " + checkIds);
        }

        var checkId = checkIds.get(0);

        var tests = db.currentOrReadOnly(() -> {
            var check = storageCache.checks().getFreshOrThrow(checkId);
            log.info("Starting {} Large tests for {}, username is {}", keys.size(), check.getId(), startedBy);

            checkLargeSettings(check);

            var foundTests = db.testDiffs().find(keys).stream()
                    .collect(Collectors.toMap(TestDiffEntity::getId, Function.identity()));

            // Make sure to start Large tests in same order as requested
            var orderedTests = new LinkedHashMap<TestDiffEntity.Id, TestDiffEntity>(keys.size());
            for (var iterator = keys.iterator(); iterator.hasNext(); ) {
                var key = iterator.next();
                var value = foundTests.get(key);
                if (value != null) {
                    orderedTests.put(key, value);
                    iterator.remove();
                }
            }

            if (!keys.isEmpty()) {
                throw new RuntimeException("Unable to find requested tests: " + keys);
            }

            return orderedTests.values().stream().map(diff -> new TestDiffWithPath(diff, null, null)).toList();
        });

        storageCache.modifyWithDbTx(cache -> {
            var check = cache.checks().getFreshOrThrow(checkId);

            var cfgRevisionSource = getConfigRevisionSource();
            var scheduled = new SchedulerImpl(check, cache, cfgRevisionSource, CheckTaskType.CTT_LARGE_TEST, startedBy)
                    .schedule(tests);

            if (scheduled) {
                requirementsService.scheduleRequirement(
                        cache,
                        check.getId(),
                        ArcanumCheckType.CI_LARGE_TESTS,
                        ArcanumCheckStatus.pending()
                );
            }
        });
    }

    LargeTestsScheduler getLargeTestsScheduler(StorageCoreCache.Modifiable cache, CheckEntity check) {
        checkLargeSettings(check);
        var configRevisionSource = getConfigRevisionSource();
        var startedBy = check.getRunLargeTestsAfterDiscoveryBy();
        return (checkTaskType, tests) ->
                new SchedulerImpl(check, cache, configRevisionSource, checkTaskType, startedBy).schedule(tests);
    }

    public void sendLogbrokerEvents(@Nonnull CheckIterationEntity.Id iterationId) {
        var result = new Object() {
            CheckEntity check;
            CheckIterationEntity heavyIteration;
            List<CheckTaskEntity> tasks;
        };
        db.currentOrReadOnly(() -> {
            result.check = storageCache.checks().getFreshOrThrow(iterationId.getCheckId());
            result.heavyIteration = storageCache.iterations().getFreshOrThrow(iterationId);
            result.tasks = db.checkTasks().getByIteration(iterationId);
        });

        var checkProto = CheckProtoMappers.toProtoCheck(result.check);
        var iterationProto = CheckProtoMappers.toProtoIteration(result.heavyIteration);

        log.info("Sending Heavy Iteration to Storage Events: {}", result.heavyIteration.getId());
        storageEventsProducer.onIterationRegistered(checkProto, iterationProto);

        log.info("Sending Heavy Tasks to Storage Events: {}", result.tasks.size());
        Preconditions.checkState(result.tasks.size() > 0, "Internal error: no heavy tasks to send");
        var tasksProto = result.tasks.stream()
                .map(CheckProtoMappers::toProtoCheckTask)
                .toList();

        storageEventsProducer.onTasksRegistered(checkProto, iterationProto, tasksProto);

    }

    private void checkLargeSettings(CheckEntity check) {
        // Make sure we not starting large tests with invalid user
        if (check.getType() == CheckOuterClass.CheckType.BRANCH_PRE_COMMIT) {
            Preconditions.checkState(check.getLargeTestsConfig() != null,
                    "Large tests configuration cannot be empty for %s checks", check.getType());
        }
    }

    private Supplier<Common.OrderedArcRevision> getConfigRevisionSource() {
        return Suppliers.memoize(() -> {
            var dir = VirtualCiProcessId.LARGE_FLOW.getDir();
            var request = StorageApi.GetLastValidConfigRequest.newBuilder()
                    .setDir(dir)
                    .setBranch(ArcBranch.trunk().getBranch())
                    .build();

            log.info("Lookup for latest {} config revision...", dir);
            return ciClient.getLastValidConfig(request).getConfigRevision();
        });
    }

    @AllArgsConstructor
    private class SchedulerImpl {
        private final CheckEntity check;
        private final StorageCoreCache.Modifiable cache;
        private final Supplier<Common.OrderedArcRevision> configRevisionSource;
        private final CheckTaskType checkTaskType;

        @Nullable // non null means manual run
        private final String startedBy;

        public boolean schedule(List<TestDiffWithPath> tests) {
            var selectedTests = selectTests(tests);
            if (selectedTests.isEmpty()) {
                log.warn("[{}] No tests were scheduled, none matched with expected status", checkTaskType);
                return false;
            } else {
                log.info("[{}] Total {} tests to schedule", checkTaskType, selectedTests.size());

                // It is possible that exactly the same large tests were registered and started already
                // Prevent double registration

                var filteredTests = filterDuplicateSelectedTests(selectedTests);
                if (filteredTests.isEmpty()) {
                    log.info("All tests were filtered as duplicate");
                    return false;
                } else {
                    log.info("Tests after filter for duplicate: {} -> {}", selectedTests.size(), filteredTests.size());
                }

                var configuredTests = matchConfigForSelectedTests(filteredTests);
                Preconditions.checkState(!configuredTests.isEmpty(),
                        "Internal error. Configured tests cannot be empty");

                var heavyIteration = registerHeavyIteration();
                scheduleSelectedLargeTests(heavyIteration, configuredTests);
                scheduleLogbrokerEvents(heavyIteration);
                return true;
            }
        }

        private List<SelectedTest> filterDuplicateSelectedTests(List<SelectedTest> tests) {
            if (checkTaskType != CheckTaskType.CTT_LARGE_TEST) {
                log.info("Skip excluding large tests for check type {}", checkTaskType);
                return tests;
            }

            var iterationType = CheckIteration.IterationType.HEAVY;
            var heavyTaskNames = db.checkTasks().findJobNames(check.getId(), iterationType)
                    .stream()
                    .map(CheckTaskTable.JobNameView::getJobName)
                    .collect(Collectors.toSet());

            return tests.stream()
                    .filter(test -> {
                        if (heavyTaskNames.contains(test.getJobName())) {
                            log.info("Exclude duplicate test {}, already registered for check {} and {} iteration type",
                                    test.getJobName(), check.getId(), iterationType);
                            return false;
                        } else {
                            return true;
                        }
                    })
                    .toList();
        }

        private List<SelectedTest> selectTests(List<TestDiffWithPath> tests) {
            var filter = new SelectDiffFilter();
            tests.forEach(filter::filterTests);
            return filter.selectedTests;
        }

        private void scheduleLogbrokerEvents(CheckIterationEntity heavyIteration) {
            log.info("Scheduling Heavy Iteration send to Storage Events: {}", heavyIteration.getId());
            bazingaTaskManager.schedule(new LargeLogbrokerTask(heavyIteration.getId()));
        }

        private List<ConfiguredTest> matchConfigForSelectedTests(List<SelectedTest> selectedTests) {
            return switch (check.getType()) {
                case BRANCH_PRE_COMMIT -> {
                    // Delegated config in branch is hardcoded
                    var delegatedConfig = buildBranchDelegatedConfig();
                    yield selectedTests.stream()
                            .map(test -> new ConfiguredTest(test, delegatedConfig))
                            .toList();
                }
                case TRUNK_PRE_COMMIT, TRUNK_POST_COMMIT -> buildTrunkBasedDelegatedConfig(selectedTests);
                default -> throw new IllegalStateException(
                        "Unsupported check type, unable to start large tests: " + check.getType()
                );
            };
        }

        private void scheduleSelectedLargeTests(
                CheckIterationEntity iteration,
                List<ConfiguredTest> configuredTests
        ) {
            var metaIterationId = cache.iterations().getFresh(iteration.getId().toMetaId())
                    .map(CheckIterationEntity::getId)
                    .orElse(null);

            log.info("Using meta iteration {}", metaIterationId);

            int taskIndex = 0;

            var testsByTaskId = new LinkedHashMap<CheckTaskEntity.Id, ConfiguredTest>(configuredTests.size());
            for (var test : configuredTests) {
                var id = new CheckTaskEntity.Id(iteration.getId(), String.valueOf(taskIndex++));
                testsByTaskId.put(id, test);
            }

            var existingTasks = cache.checkTasks().getFresh(testsByTaskId.keySet());
            log.info("{} tasks already exists", existingTasks.size());

            var largeTestCache = new LargeTestCache();
            for (var entry : testsByTaskId.entrySet()) {
                var id = entry.getKey();
                var test = entry.getValue();
                var checkTask = registerHeavyTask(iteration.getId(), metaIterationId, test.selectedTest, id);
                new LargeTaskProcessor(test.selectedTest, test.delegatedConfig, checkTask, largeTestCache)
                        .register();
            }

            log.info("All {} large tests are prepared", configuredTests.size());
        }

        private CheckTaskEntity registerHeavyTask(
                CheckIterationEntity.Id iterationId,
                @Nullable CheckIterationEntity.Id metaIterationId,
                SelectedTest diff,
                CheckTaskEntity.Id taskId
        ) {
            var checkTask = checkService.registerNotExistingTaskInTx(
                    cache,
                    CheckTaskEntity.builder()
                            .id(taskId)
                            .numberOfPartitions(1)
                            .right(diff.isRight())
                            .status(ru.yandex.ci.storage.core.Common.CheckStatus.CREATED)
                            .jobName(diff.getJobName())
                            .completedPartitions(Set.of())
                            .created(Instant.now())
                            .type(checkTaskType)
                            .build(),
                    iterationId,
                    metaIterationId
            );

            log.info("Large auto-start task is registered: {}", checkTask.getId());
            return checkTask;
        }

        private CheckIterationEntity registerHeavyIteration() {
            // Note: it is not required for provide expected jobs, we sure don't expect to lost anything here
            var heavyIteration = checkService.registerIterationInTx(
                    cache,
                    CheckIterationEntity.Id.of(
                            check.getId(),
                            CheckIteration.IterationType.HEAVY,
                            0 // Simply `next iteration`
                    ),
                    CreateIterationParams.builder()
                            .startedBy(startedBy)
                            .tasksType(checkTaskType)
                            .build()
            );

            log.info("Large auto-start iteration is registered: {}", heavyIteration.getId());
            return heavyIteration;
        }

        private StorageApi.DelegatedConfig buildBranchDelegatedConfig() {
            var largeTestsConfig = check.getLargeTestsConfig();
            log.info("Looking for exact large tests config: {}", largeTestsConfig);

            Preconditions.checkState(largeTestsConfig != null,
                    "Internal error, large test configuration is required for %s", check.getType());

            var revision = largeTestsConfig.getRevision();
            return StorageApi.DelegatedConfig.newBuilder()
                    .setPath(largeTestsConfig.getPath())
                    .setRevision(Common.OrderedArcRevision.newBuilder()
                            .setBranch(revision.getBranch())
                            .setHash(revision.getRevision())
                            .setNumber(revision.getRevisionNumber())
                    ).build();
        }

        private List<ConfiguredTest> buildTrunkBasedDelegatedConfig(List<SelectedTest> selectedTests) {
            var revision = check.getLeft();
            var revisionNumber = revision.getRevisionNumber();
            log.info("Looking for path based configs starting from revision {}", revisionNumber);

            Preconditions.checkState(revisionNumber > 0,
                    "Trunk based configs must be based from trunk revision, found %s", revision);

            Function<SelectedTest, String> getPath = test -> {
                var diff = test.getTestDiffWithPath();
                return diff.getConfigPath() != null
                        ? diff.getConfigPath()
                        : diff.getTestDiff().getId().getPath();
            };

            var prefixes = selectedTests.stream()
                    .map(getPath)
                    .collect(Collectors.toSet());

            boolean ignoreConfigsWithSecurityProblems = startedBy != null ||
                    check.getType() == CheckOuterClass.CheckType.TRUNK_POST_COMMIT;

            var request = StorageApi.GetLastValidPrefixedConfigRequest.newBuilder()
                    .setRevision(revisionNumber)
                    .addAllPrefixDir(prefixes)
                    .setIgnoreSecurityProblems(ignoreConfigsWithSecurityProblems)
                    .build();
            var response = ciClient.getLastValidConfigBatch(request);

            var configMap = new HashMap<String, StorageApi.DelegatedConfig>(response.getResponsesCount());
            for (var resp : response.getResponsesList()) {
                configMap.computeIfAbsent(resp.getPrefixDir(), prefix -> StorageApi.DelegatedConfig.newBuilder()
                        .setPath(resp.getPath())
                        .setRevision(Common.OrderedArcRevision.newBuilder()
                                .setBranch(resp.getConfigRevision().getBranch())
                                .setHash(resp.getConfigRevision().getHash())
                                .setNumber(resp.getConfigRevision().getNumber())
                        ).build());
            }

            return selectedTests.stream()
                    .map(test -> new ConfiguredTest(test,
                            configMap.getOrDefault(getPath.apply(test),
                                    StorageApi.DelegatedConfig.getDefaultInstance()))
                    ).collect(Collectors.toList());
        }

        private class SelectDiffFilter {
            private final List<SelectedTest> selectedTests;
            private final TestStatus testStatusMatch;

            private SelectDiffFilter() {
                this.selectedTests = new ArrayList<>(16);
                this.testStatusMatch = matchCheckTaskType(checkTaskType, TestStatus.TS_DISCOVERED, TestStatus.TS_OK);
            }

            void filterTests(TestDiffWithPath testDiffWithPath) {
                var testDiff = testDiffWithPath.getTestDiff();
                log.info("Starting Large test for {}", testDiff.getId());

                var leftStatus = testDiff.getLeft();
                if (leftStatus == testStatusMatch) {
                    log.info("With Left");
                    selectedTests.add(toSelectedTest(testDiffWithPath, false));
                } else {
                    log.info("Skip Left {}", leftStatus);
                }

                var rightStatus = testDiff.getRight();
                if (rightStatus == testStatusMatch) {
                    log.info("With Right");
                    selectedTests.add(toSelectedTest(testDiffWithPath, true));
                } else {
                    log.info("Skip Right {}", rightStatus);
                }
            }

            private SelectedTest toSelectedTest(TestDiffWithPath testDiffWithPath, boolean right) {
                return new SelectedTest(testDiffWithPath, right, testDiffWithPath.toJobName(checkTaskType));
            }
        }

        @SuppressWarnings("UnstableApiUsage")
        @RequiredArgsConstructor
        private class LargeTaskProcessor {
            private final SelectedTest diff;
            private final StorageApi.DelegatedConfig delegatedConfig;
            private final CheckTaskEntity checkTask;
            private final LargeTestCache largeTestCache;

            void register() {
                var testDiff = diff.getTestDiffWithPath().getTestDiff();
                var testDiffId = testDiff.getId();
                var prevEntity = largeTestCache.cache.get(testDiffId);

                LargeTaskEntity.Builder builder;
                if (prevEntity == null) {
                    var id = new LargeTaskEntity.Id(
                            checkTask.getId().getIterationId(),
                            checkTaskType,
                            largeTestCache.index++
                    );
                    log.info("Registering Large Task: {}", id);

                    var delegatedConfigValue = CheckProtoMappers.hasDelegatedConfig(delegatedConfig)
                            ? CheckProtoMappers.toDelegatedConfig(delegatedConfig)
                            : null;
                    if (checkTaskType == CheckTaskType.CTT_NATIVE_BUILD) {
                        Preconditions.checkState(diff.getTestDiffWithPath().getNativeDir() != null,
                                "Internal error. Native dir must be configured for check task type %s", checkTaskType);
                    }

                    builder = LargeTaskEntity.builder()
                            .id(id)
                            .configRevision(CiCoreProtoMappers.toOrderedArcRevision(configRevisionSource.get()))
                            .delegatedConfig(delegatedConfigValue)
                            .startedBy(startedBy)
                            .target(testDiffId.getPath())
                            .nativeTarget(diff.getTestDiffWithPath().getNativeDir());

                    if (Strings.isNotEmpty(diff.getTestDiffWithPath().getNativeDir())) {
                        var targets = testDiffId.getPath().split(";");
                        builder = builder.nativeSpecification(
                                NativeSpecification.builder()
                                        .targets(toNativeTargets(testDiffId, targets))
                                        .build()
                        );
                    }
                } else {
                    builder = prevEntity.toBuilder();
                }

                var taskId = checkTask.getId().getTaskId();
                var largeTestInfo = prepareTestInfo(testDiff);
                if (diff.isRight()) {
                    builder.rightTaskId(taskId);
                    builder.rightLargeTestInfo(largeTestInfo);
                } else {
                    builder.leftTaskId(taskId);
                    builder.leftLargeTestInfo(largeTestInfo);
                }

                var newEntity = builder.build();
                largeTestCache.cache.put(testDiffId, newEntity);

                cache.largeTasks().writeThrough(newEntity);

                if (prevEntity == null) {
                    log.info("Starting Large Flow Task: {}", newEntity.getId());
                    var task = new LargeFlowTask(newEntity.getId());
                    bazingaTaskManager.schedule(
                            task,
                            task.getTaskCategory(),
                            org.joda.time.Instant.now(),
                            task.priority(),
                            true
                    ); // Make sure it's unique
                }
            }

            private Map<String, NativeSpecification.Target> toNativeTargets(
                    TestDiffEntity.Id testDiffId,
                    String[] targets
            ) {
                return Arrays.stream(targets).collect(
                        Collectors.toMap(
                                Function.identity(),
                                target -> NativeSpecification.Target.builder()
                                        .hid(generateNativeHid(target, testDiffId.getToolchain()))
                                        .build()
                        )
                );
            }

            private String generateNativeHid(String target, String toolchain) {
                var hash = Hashing.sipHash24().newHasher()
                        .putString(target, StandardCharsets.UTF_8)
                        .putString(toolchain, StandardCharsets.UTF_8)
                        .hash()
                        .asLong();
                return UnsignedLong.fromLongBits(hash).toString();
            }
        }
    }

    public static <T> T matchCheckTaskType(CheckTaskType taskType, T largeTestsSource, T nativeBuildsSource) {
        return switch (taskType) {
            case CTT_LARGE_TEST -> largeTestsSource;
            case CTT_NATIVE_BUILD -> nativeBuildsSource;
            case UNRECOGNIZED, CTT_AUTOCHECK, CTT_TESTENV -> throw new IllegalStateException("Unsupported " + taskType);
        };
    }

    public static boolean isPrecommit(CheckOuterClass.CheckType type) {
        return switch (type) {
            case BRANCH_PRE_COMMIT, TRUNK_PRE_COMMIT -> true;
            case BRANCH_POST_COMMIT, TRUNK_POST_COMMIT -> false;
            case UNRECOGNIZED -> throw new RuntimeException("Unsupported check type: " + type);
        };
    }

    private static LargeTestInfo prepareTestInfo(TestDiffEntity testDiff) {
        var id = testDiff.getId();

        var requirements = testDiff.getRequirements();

        JsonNode requirementsMap = StringUtils.isEmpty(requirements)
                ? CiJson.mapper().createObjectNode()
                : parseRequirements(requirements);

        return new LargeTestInfo(
                id.getToolchain(),
                testDiff.getTagsList(),
                testDiff.getName(),
                Objects.requireNonNullElse(testDiff.getOldSuiteId(), ""),
                UnsignedLong.fromLongBits(id.getSuiteId()),
                requirementsMap
        );
    }

    private static JsonNode parseRequirements(String requirements) {
        try {
            return CiJson.mapper().readTree(requirements);
        } catch (JsonProcessingException e) {
            throw new RuntimeException("Unable to parse requirements: " + requirements, e);
        }
    }


    // We use this cache to update LargeTaskEntity (2 sides for large tests)
    private static class LargeTestCache {
        // This is much more efficient then loading records from database for each large test
        private final Map<TestDiffEntity.Id, LargeTaskEntity> cache = new HashMap<>();
        private int index;
    }

    @Value
    public static class TestDiffWithPath {
        @Nonnull
        TestDiffEntity testDiff;
        @Nullable
        String configPath;
        @Nullable
        String nativeDir;

        public String toJobName(CheckTaskType checkTaskType) {
            var ciProcessId = CheckTaskTypeUtils.toCiProcessId(
                    checkTaskType,
                    testDiff.getId().getPath(),
                    nativeDir,
                    testDiff.getId().getToolchain(),
                    testDiff.getName()
            );
            return ciProcessId.asString();
        }
    }

    @Value
    private static class SelectedTest {
        @Nonnull
        TestDiffWithPath testDiffWithPath;
        boolean right;
        @Nonnull
        String jobName;
    }

    @Value
    private static class ConfiguredTest {
        @Nonnull
        SelectedTest selectedTest;
        @Nonnull
        StorageApi.DelegatedConfig delegatedConfig;
    }

    interface LargeTestsScheduler {
        boolean schedule(CheckTaskType checkTaskType, List<TestDiffWithPath> tests);
    }
}
