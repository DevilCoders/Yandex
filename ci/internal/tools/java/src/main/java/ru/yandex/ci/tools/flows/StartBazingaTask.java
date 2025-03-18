package ru.yandex.ci.tools.flows;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.common.bazinga.spring.BazingaCoreConfig;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.check_task.LargeTaskEntity;
import ru.yandex.ci.storage.core.large.LargeFlowTask;
import ru.yandex.ci.tools.AbstractSpringBasedApp;
import ru.yandex.commune.bazinga.BazingaTaskManager;

@Configuration
@Import(BazingaCoreConfig.class)
public class StartBazingaTask extends AbstractSpringBasedApp {

    @Autowired
    BazingaTaskManager bazingaTaskManager;

    @Override
    protected void run() {
        var id = new LargeTaskEntity.Id(
                CheckIterationEntity.Id.of(CheckEntity.Id.of(57600000001418L), CheckIteration.IterationType.HEAVY, 1),
                Common.CheckTaskType.CTT_LARGE_TEST,
                0
        );
        var task = new LargeFlowTask(id);
        bazingaTaskManager.schedule(
                task,
                task.getTaskCategory(),
                org.joda.time.Instant.now(),
                task.priority(),
                true // Make sure it's unique
        );
    }

    public static void main(String[] args) {
        startAndStopThisClass(args, "ci-storage", "ci-storage-tms");
    }
}
