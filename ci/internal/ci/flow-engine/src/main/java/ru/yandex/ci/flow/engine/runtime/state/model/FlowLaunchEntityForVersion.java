package ru.yandex.ci.flow.engine.runtime.state.model;

import javax.annotation.Nonnull;

import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.repository.db.Table.View;
import yandex.cloud.repository.kikimr.DbType;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.launch.versioning.Version;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;

@Value
public class FlowLaunchEntityForVersion implements View {

    @Column
    @Nonnull
    FlowLaunchEntity.Id id;

    @Column(dbType = DbType.UTF8)
    @Nonnull
    String processId;

    @Column
    int launchNumber;

    @Column(dbType = DbType.JSON, flatten = false)
    @Nonnull
    LaunchInfo launchInfo;

    public FlowLaunchId getFlowLaunchId() {
        return FlowLaunchId.of(id.getId());
    }

    public LaunchId getLaunchId() {
        return LaunchId.of(CiProcessId.ofString(processId), launchNumber);
    }

    public Version getVersion() {
        return launchInfo.getVersion();
    }
}
