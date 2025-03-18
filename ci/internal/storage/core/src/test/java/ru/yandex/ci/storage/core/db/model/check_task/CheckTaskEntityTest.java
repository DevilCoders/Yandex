package ru.yandex.ci.storage.core.db.model.check_task;

import org.assertj.core.api.Assertions;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;

class CheckTaskEntityTest {

    @Test
    void getLogbrokerSourceId() {
        var checkId = CheckEntity.Id.of(4221L);
        var iterationId = CheckIterationEntity.Id.of(checkId, CheckIteration.IterationType.FAST, 1);
        var checkTaskId = new CheckTaskEntity.Id(iterationId, "task-id");
        var checkTask = CheckTaskEntity.builder().id(checkTaskId).build();
        Assertions.assertThat(checkTask.getLogbrokerSourceId()).isEqualTo("4221/FAST/1/task-id");
    }
}
