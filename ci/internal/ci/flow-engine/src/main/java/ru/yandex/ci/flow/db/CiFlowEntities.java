package ru.yandex.ci.flow.db;

import java.util.List;

import yandex.cloud.repository.db.Entity;
import ru.yandex.ci.core.job.JobInstance;
import ru.yandex.ci.flow.engine.runtime.di.ResourceEntity;
import ru.yandex.ci.flow.engine.runtime.state.StageGroupState;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;

public class CiFlowEntities {

    @SuppressWarnings("rawtypes")
    public static final List<Class<? extends Entity>> ALL = List.of(
            JobInstance.class,
            ResourceEntity.class,
            StageGroupState.class,
            FlowLaunchEntity.class
    );

    private CiFlowEntities() {
    }
}
