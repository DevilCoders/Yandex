package ru.yandex.ci.engine.discovery.arc_reflog;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.EngineTestBase;
import ru.yandex.commune.bazinga.BazingaTaskManager;
import ru.yandex.commune.bazinga.impl.BazingaTaskManagerImpl;
import ru.yandex.commune.bazinga.impl.TaskId;
import ru.yandex.commune.bazinga.impl.storage.memory.InMemoryBazingaStorage;
import ru.yandex.misc.db.q.SqlLimits;

import static org.assertj.core.api.Assertions.assertThat;

class ReflogProcessorServiceTest extends EngineTestBase {

    private BazingaTaskManager taskManager;

    @BeforeEach
    void setUp() {
        InMemoryBazingaStorage bazingaStorage = new InMemoryBazingaStorage();
        taskManager = new BazingaTaskManagerImpl(bazingaStorage);
    }

    @Test
    void skipUserBranches() {

        reflogProcessorService.processReflogRecord(
                ArcBranch.ofBranchName("user/rembo/uber-fix-CI-9000"),
                TestData.DS2_REVISION, TestData.TRUNK_R5.toRevision()
        );

        assertThat(taskManager.getActiveJobs(TaskId.from(ProcessArcReflogRecordTask.class), SqlLimits.all()))
                .isEmpty();
    }


}
