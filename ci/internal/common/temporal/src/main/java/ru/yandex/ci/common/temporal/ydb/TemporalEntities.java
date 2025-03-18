package ru.yandex.ci.common.temporal.ydb;

import java.util.List;

import yandex.cloud.repository.db.Entity;
import ru.yandex.ci.common.temporal.monitoring.TemporalFailingWorkflowEntity;

public class TemporalEntities {

    @SuppressWarnings("rawtypes")
    public static final List<Class<? extends Entity>> ALL = List.of(
            TemporalLaunchQueueEntity.class,
            TemporalFailingWorkflowEntity.class
    );

    private TemporalEntities() {
    }
}
