package ru.yandex.ci.observer.core.db;

import java.util.List;

import yandex.cloud.repository.db.Entity;
import ru.yandex.ci.observer.core.db.model.check.CheckEntity;
import ru.yandex.ci.observer.core.db.model.check_iterations.CheckIterationEntity;
import ru.yandex.ci.observer.core.db.model.check_tasks.CheckTaskEntity;
import ru.yandex.ci.observer.core.db.model.settings.ObserverSettings;
import ru.yandex.ci.observer.core.db.model.sla_statistics.SlaStatisticsEntity;
import ru.yandex.ci.observer.core.db.model.stress_test.StressTestUsedCommitEntity;
import ru.yandex.ci.observer.core.db.model.traces.CheckTaskPartitionTraceEntity;

@SuppressWarnings("rawtypes")
public class CiObserverEntities {
    private static final List<Class<? extends Entity>> ENTITIES = List.of(
            ObserverSettings.class,
            CheckEntity.class,
            CheckIterationEntity.class,
            CheckTaskEntity.class,
            CheckTaskPartitionTraceEntity.class,
            SlaStatisticsEntity.class,
            StressTestUsedCommitEntity.class
    );

    private CiObserverEntities() {
    }

    public static List<Class<? extends Entity>> getEntities() {
        return ENTITIES;
    }
}
