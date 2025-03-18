package ru.yandex.ci.storage.core;

import java.time.Instant;
import java.util.Set;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.test.context.ContextConfiguration;
import org.springframework.test.context.TestExecutionListeners;

import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.CiStorageRepository;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.IterationInfo;
import ru.yandex.ci.storage.core.db.model.check_iteration.IterationStatistics;
import ru.yandex.ci.storage.core.db.model.check_task.CheckTaskEntity;
import ru.yandex.ci.storage.core.db.model.common.StorageRevision;
import ru.yandex.ci.storage.core.spring.StorageYdbTestConfig;
import ru.yandex.ci.test.YdbCleanupTestListener;

@ContextConfiguration(classes = {
        StorageYdbTestConfig.class
})
@TestExecutionListeners(
        value = {YdbCleanupTestListener.class},
        mergeMode = TestExecutionListeners.MergeMode.MERGE_WITH_DEFAULTS
)
public class StorageYdbTestBase extends StorageTestBase {
    protected final CheckEntity.Id sampleCheckId = CheckEntity.Id.of(1L);
    protected final CheckIterationEntity.Id sampleIterationId = CheckIterationEntity.Id.of(
            sampleCheckId, CheckIteration.IterationType.FULL, 1
    );

    protected final CheckEntity sampleCheck = CheckEntity.builder()
            .id(sampleCheckId)
            .left(new StorageRevision("trunk", "left", 0, Instant.EPOCH))
            .right(new StorageRevision("trunk", "right", 0, Instant.EPOCH))
            .diffSetId(1L)
            .author("author")
            .created(Instant.now())
            .type(CheckOuterClass.CheckType.TRUNK_POST_COMMIT)
            .build();

    protected final CheckIterationEntity sampleIteration = CheckIterationEntity.builder()
            .id(sampleIterationId)
            .status(Common.CheckStatus.CREATED)
            .created(Instant.now())
            .build();


    protected final CheckTaskEntity sampleTask = CheckTaskEntity.builder()
            .id(new CheckTaskEntity.Id(sampleIterationId, "task-id"))
            .right(true)
            .jobName("job_name")
            .completedPartitions(Set.of())
            .numberOfPartitions(2)
            .status(Common.CheckStatus.CREATED)
            .created(Instant.now())
            .build();

    @Autowired
    protected CiStorageDb db;

    @Autowired
    protected CiStorageRepository repository;

    protected static CheckEntity createTestCheck() {
        return CheckEntity.builder()
                .id(CheckEntity.Id.of(1L))
                .left(StorageRevision.EMPTY)
                .right(StorageRevision.EMPTY)
                .type(CheckOuterClass.CheckType.TRUNK_POST_COMMIT)
                .created(Instant.EPOCH)
                .build();
    }

    protected static CheckIterationEntity createTestIteration() {
        return CheckIterationEntity.builder()
                .id(CheckIterationEntity.Id.of(CheckEntity.Id.of(1L), CheckIteration.IterationType.FAST, 1))
                .status(Common.CheckStatus.RUNNING)
                .statistics(IterationStatistics.EMPTY)
                .info(IterationInfo.EMPTY)
                .build();
    }
}

