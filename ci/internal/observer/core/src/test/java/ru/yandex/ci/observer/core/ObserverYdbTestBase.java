package ru.yandex.ci.observer.core;

import java.time.Instant;
import java.util.Map;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.test.context.ContextConfiguration;
import org.springframework.test.context.TestExecutionListeners;

import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.observer.core.db.CiObserverDb;
import ru.yandex.ci.observer.core.db.CiObserverRepository;
import ru.yandex.ci.observer.core.db.model.check.CheckEntity;
import ru.yandex.ci.observer.core.db.model.check_iterations.CheckIterationEntity;
import ru.yandex.ci.observer.core.db.model.check_iterations.LinkName;
import ru.yandex.ci.observer.core.db.model.check_tasks.CheckTaskEntity;
import ru.yandex.ci.observer.core.spring.ObserverYdbTestConfig;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.CheckOuterClass;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.model.common.StorageRevision;
import ru.yandex.ci.test.YdbCleanupTestListener;
import ru.yandex.ci.test.clock.OverridableClock;

@ContextConfiguration(classes = {
        ObserverYdbTestConfig.class
})
@TestExecutionListeners(
        value = {YdbCleanupTestListener.class},
        mergeMode = TestExecutionListeners.MergeMode.MERGE_WITH_DEFAULTS
)
public class ObserverYdbTestBase extends ObserverTestBase {
    protected static final Instant TIME = Instant.parse("2021-10-20T12:00:00Z");
    protected static final CheckEntity.Id SAMPLE_CHECK_ID = CheckEntity.Id.of(1L);
    protected static final CheckIterationEntity.Id SAMPLE_ITERATION_ID = new CheckIterationEntity.Id(
            SAMPLE_CHECK_ID, CheckIteration.IterationType.FULL, 1
    );
    protected static final CheckIterationEntity.Id SAMPLE_ITERATION_ID_2 = new CheckIterationEntity.Id(
            SAMPLE_CHECK_ID, CheckIteration.IterationType.FULL, 2
    );
    protected static final CheckTaskEntity.Id SAMPLE_TASK_ID = new CheckTaskEntity.Id(
            SAMPLE_ITERATION_ID, "testTaskId"
    );
    protected static final CheckTaskEntity.Id SAMPLE_TASK_ID_2 = new CheckTaskEntity.Id(
            SAMPLE_ITERATION_ID, "testTaskId2"
    );
    protected static final CheckEntity SAMPLE_CHECK = CheckEntity.builder()
            .id(SAMPLE_CHECK_ID)
            .left(StorageRevision.EMPTY)
            .right(StorageRevision.from(
                    "pr:1234",
                    ArcCommit.builder().createTime(TIME).id(ArcCommit.Id.of("commitId")).svnRevision(123).build()
            ))
            .diffSetId(123L)
            .created(TIME)
            .type(CheckOuterClass.CheckType.TRUNK_PRE_COMMIT)
            .author("anmakon")
            .status(Common.CheckStatus.RUNNING)
            .diffSetEventCreated(Instant.EPOCH)
            .build();
    protected static final CheckIterationEntity SAMPLE_ITERATION = CheckIterationEntity.builder()
            .id(SAMPLE_ITERATION_ID)
            .created(TIME)
            .checkType(CheckOuterClass.CheckType.TRUNK_PRE_COMMIT)
            .author("anmakon")
            .left(StorageRevision.EMPTY)
            .right(StorageRevision.from(
                    "pr:1234",
                    ArcCommit.builder().createTime(TIME).id(ArcCommit.Id.of("commitId")).svnRevision(123).build()
            ))
            .status(Common.CheckStatus.RUNNING)
            .advisedPool("test_pool")
            .testenvId("")
            .checkRelatedLinks(Map.of(
                    LinkName.CI_BADGE,
                    "https://a.yandex-team.ru/ci-card-preview/1",
                    LinkName.SANDBOX,
                    "https://sandbox.yandex-team.ru/tasks?limit=100&hints=1%2FFULL&tags=AUTOCHECK",
                    LinkName.DISTBUILD,
                    "https://datalens.yandex-team.ru/0e0iocvqdgsuv-distbuildprofiler?review=1234",
                    LinkName.REVIEW,
                    "https://a.yandex-team.ru/review/1234/details",
                    LinkName.DISTBUILD_VIEWER,
                    "https://viewer-distbuild.n.yandex-team.ru/search?query=1"
            ))
            .pessimized(true)
            .diffSetEventCreated(Instant.EPOCH)
            .build();
    protected static final CheckTaskEntity SAMPLE_TASK = CheckTaskEntity.builder()
            .id(SAMPLE_TASK_ID)
            .checkType(CheckOuterClass.CheckType.TRUNK_PRE_COMMIT)
            .created(TIME)
            .rightRevisionTimestamp(TIME)
            .numberOfPartitions(3)
            .status(Common.CheckStatus.RUNNING)
            .jobName("TEST_JOB_NAME")
            .build();
    protected static final CheckTaskEntity SAMPLE_TASK_2 = CheckTaskEntity.builder()
            .id(SAMPLE_TASK_ID_2)
            .checkType(CheckOuterClass.CheckType.TRUNK_PRE_COMMIT)
            .created(TIME)
            .rightRevisionTimestamp(TIME)
            .numberOfPartitions(1)
            .status(Common.CheckStatus.RUNNING)
            .jobName("TEST_JOB_NAME_2")
            .build();

    @Autowired
    protected CiObserverDb db;

    @Autowired
    protected CiObserverRepository repository;

    @Autowired
    protected OverridableClock clock;
}
