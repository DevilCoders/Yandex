package ru.yandex.ci.flow.db;

import ru.yandex.ci.core.job.JobInstanceTable;
import ru.yandex.ci.flow.engine.runtime.di.ResourceTable;
import ru.yandex.ci.flow.engine.runtime.state.FlowLaunchTable;
import ru.yandex.ci.flow.engine.runtime.state.StageGroupTable;

public interface CiFlowTables {

    JobInstanceTable jobInstance();

    ResourceTable resources();

    StageGroupTable stageGroup();

    FlowLaunchTable flowLaunch();
}
