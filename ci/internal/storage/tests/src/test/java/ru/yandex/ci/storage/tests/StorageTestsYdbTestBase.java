package ru.yandex.ci.storage.tests;

import java.time.Instant;
import java.util.List;
import java.util.Map;
import java.util.Set;

import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeEach;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.test.context.ContextConfiguration;

import ru.yandex.ci.common.proto.ProtoConverter;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.storage.core.Actions;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.StorageYdbTestBase;
import ru.yandex.ci.storage.core.TaskMessages;
import ru.yandex.ci.storage.core.cache.StorageCoreCache;
import ru.yandex.ci.storage.core.db.constant.Trunk;
import ru.yandex.ci.storage.core.db.model.common.StorageRevision;
import ru.yandex.ci.storage.core.sharding.ChunkDistributor;
import ru.yandex.ci.storage.post_processor.PostProcessorStatistics;
import ru.yandex.ci.storage.tests.logbroker.TestLogbrokerService;
import ru.yandex.ci.storage.tests.spring.StorageTestsConfig;
import ru.yandex.ci.storage.tests.tester.StorageTester;

@ContextConfiguration(classes = StorageTestsConfig.class)
public class StorageTestsYdbTestBase extends StorageYdbTestBase {
    public static final String TASK_ID_LEFT = "task-id-left";
    public static final String TASK_ID_RIGHT = "task-id-right";
    public static final String TASK_ID_LEFT_TWO = "task-id-left-two";
    public static final String TASK_ID_RIGHT_TWO = "task-id-right-two";
    protected static final String TASK_ID_LEFT_THREE = "task-id-left-three";
    protected static final String TASK_ID_RIGHT_THREE = "task-id-right-three";
    protected static final String DISTBUILD_STARTED = "distbuild/started";

    @Autowired
    protected TestLogbrokerService logbrokerService;

    @Autowired
    protected StorageTester storageTester;

    @Autowired
    protected List<StorageCoreCache<?>> caches;

    @Autowired
    protected TestArcanumClient testArcanumClient;

    @Autowired
    protected TestCiClient testCiClient;

    @Autowired
    protected TestsArcService arcService;

    @Autowired
    protected TestAYamlerClientImpl aYamlerClient;

    @Autowired
    protected PostProcessorStatistics postProcessorStatistics;

    @Autowired
    protected BadgeEventsSenderTestImpl badgeEventsSender;

    @Autowired
    protected ChunkDistributor chunkDistributor;

    @BeforeEach
    public void setUp() {
        storageTester.initialize();
        logbrokerService.reset();
        testArcanumClient.reset();
        testCiClient.reset();
        aYamlerClient.disableStrongMode();
        postProcessorStatistics.reset();
        clock.setTime(Instant.now());
        badgeEventsSender.clear();
        chunkDistributor.initializeOrCheckState();
    }

    @AfterEach
    public void tearDown() {
        storageTester.ensureAllReadsCommited();

        for (var cache : caches) {
            cache.modify(StorageCoreCache.Modifiable::invalidateAll);
        }
    }

    protected StorageRevision pr(int number, String revision) {
        return StorageRevision.from("pr:" + number, arcService.getCommit(ArcRevision.of(revision)));
    }

    protected StorageRevision r(long revision) {
        return StorageRevision.from(Trunk.name(), arcService.getCommit(ArcRevision.of("r" + revision)));
    }

    protected StorageRevision r(String branch, String revision) {
        return new StorageRevision(branch, revision, 0, Instant.now());
    }

    public TaskMessages.AutocheckTestResult exampleBuild(long hid) {
        return TaskMessages.AutocheckTestResult.newBuilder()
                .setId(
                        TaskMessages.AutocheckTestId.newBuilder()
                                .setHid(hid)
                                .setSuiteHid(hid)
                                .setToolchain("build-toolchain")
                                .build()
                )
                .setPath("ci")
                .setName("java")
                .setTestStatus(Common.TestStatus.TS_OK)
                .build();
    }

    public TaskMessages.AutocheckTestResult exampleFailedBuild(long hid) {
        return exampleBuild(hid)
                .toBuilder()
                .setTestStatus(Common.TestStatus.TS_FAILED)
                .build();
    }

    public TaskMessages.AutocheckTestResult exampleSmallTest(long hid, long suiteHid) {
        return exampleTest(hid, suiteHid, Common.ResultType.RT_TEST_SMALL);
    }

    public TaskMessages.AutocheckTestResult exampleMediumTest(long hid, long suiteHid) {
        return exampleTest(hid, suiteHid, Common.ResultType.RT_TEST_MEDIUM);
    }

    public TaskMessages.AutocheckTestResult exampleLargeTestSuite(long suiteHid) {
        return exampleTest(suiteHid, suiteHid, Common.ResultType.RT_TEST_SUITE_LARGE);
    }

    public TaskMessages.AutocheckTestResult exampleTest(long hid, long suiteHid, Common.ResultType resultType) {
        return exampleTest(hid, 0, suiteHid, resultType);
    }

    public TaskMessages.AutocheckTestResult exampleChunk(long hid, long suiteHid, Common.ResultType resultType) {
        return exampleTest(hid, hid, suiteHid, resultType).toBuilder()
                .setName("chunk 1/1")
                .setSubtestName("example_chunk")
                .build();
    }

    public TaskMessages.AutocheckTestResult exampleTest(
            long hid, long chunkHid, long suiteHid, Common.ResultType resultType
    ) {
        return TaskMessages.AutocheckTestResult.newBuilder()
                .setId(
                        TaskMessages.AutocheckTestId.newBuilder()
                                .setHid(hid)
                                .setSuiteHid(suiteHid)
                                .setToolchain("test-toolchain")
                                .build()
                )
                .setName("test")
                .setSubtestName("sub")
                .setPath("ci")
                .setChunkHid(chunkHid)
                .setResultType(resultType)
                .setTestStatus(Common.TestStatus.TS_OK)
                .setLaunchable(true)
                .setOwners(
                        TaskMessages.Owners.newBuilder()
                                .addLogins("user42")
                                .addGroups("ci")
                                .build()
                )
                .build();
    }

    public TaskMessages.AutocheckTestResult exampleSuite(long hid) {
        return TaskMessages.AutocheckTestResult.newBuilder()
                .setId(
                        TaskMessages.AutocheckTestId.newBuilder()
                                .setHid(hid)
                                .setSuiteHid(hid)
                                .setToolchain("test-toolchain")
                                .build()
                )
                .setPath("ci")
                .setName("example_suite")
                .setTestStatus(Common.TestStatus.TS_OK)
                .setResultType(Common.ResultType.RT_TEST_SUITE_MEDIUM)
                .setOwners(
                        TaskMessages.Owners.newBuilder()
                                .addLogins("user42")
                                .addGroups("ci")
                                .build()
                )
                .setOldId("old-id")
                .setOldSuiteId("old-suite-id")
                .addAllTags(Set.of("tag_1", "tag_2"))
                .setLinks(
                        TaskMessages.Links.newBuilder()
                                .putAllLink(
                                        Map.of(
                                                "log",
                                                TaskMessages.Links.Link.newBuilder()
                                                        .addLink("http://log_1")
                                                        .addLink("http://log_2")
                                                        .build()
                                        )
                                )
                                .build()
                )
                .build();
    }

    public Actions.AutocheckFatalError exampleAutocheckFatalError() {
        return Actions.AutocheckFatalError.newBuilder()
                .setTimestamp(ProtoConverter.convert(Instant.now()))
                .setMessage("The check will be incomplete due to autocheck internal errors")
                .setDetails("ymake failed to build graph")
                .setSandboxTaskId("task-id")
                .build();
    }

    public TaskMessages.AutocheckTestResult exampleNativeBuild(long hid) {
        return TaskMessages.AutocheckTestResult.newBuilder()
                .setId(
                        TaskMessages.AutocheckTestId.newBuilder()
                                .setHid(hid)
                                .setSuiteHid(hid)
                                .setToolchain("test-toolchain")
                                .build()
                )
                .setPath("ci")
                .setName("native_build")
                .setTestStatus(Common.TestStatus.TS_OK)
                .setResultType(Common.ResultType.RT_NATIVE_BUILD)
                .build();
    }
}
