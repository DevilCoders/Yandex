package ru.yandex.ci.flow.engine.runtime.state;

import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Set;
import java.util.function.Function;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.db.Table;
import yandex.cloud.repository.kikimr.DbType;
import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;
import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.launch.LaunchFlowInfo;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.launch.LaunchVcsInfo;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntityForVersion;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntityToLaunchId;
import ru.yandex.ci.util.jackson.SerializationUtils;

public class FlowLaunchTable extends KikimrTableCi<FlowLaunchEntity> {

    public FlowLaunchTable(QueryExecutor executor) {
        super(FlowLaunchEntity.class, executor);
    }

    public FlowLaunchEntity get(FlowLaunchId id) {
        return get(FlowLaunchEntity.Id.of(id.asString()));
    }

    public FlowLaunchAccessView getLaunchAccessView(FlowLaunchId id) {
        return get(FlowLaunchAccessView.class, FlowLaunchEntity.Id.of(id.asString()));
    }

    public FlowLaunchView getLaunchView(FlowLaunchId id) {
        return get(FlowLaunchView.class, FlowLaunchEntity.Id.of(id.asString()));
    }

    public Optional<FlowLaunchEntity> findOptional(FlowLaunchId id) {
        return find(FlowLaunchEntity.Id.of(id.asString()));
    }

    @Override
    public FlowLaunchEntity save(FlowLaunchEntity flowLaunchEntity) {
        return super.save(flowLaunchEntity.withIncrementedVersion());
    }

    public List<FlowLaunchEntityToLaunchId> findStageDetails(Set<FlowLaunchEntity.Id> ids) {
        return find(FlowLaunchEntityToLaunchId.class, YqlPredicate.where("id").in(ids));
    }

    public Map<FlowLaunchId, FlowLaunchEntityForVersion> findLaunchIds(Set<FlowLaunchId> flowLaunchIds) {
        var keys = flowLaunchIds.stream()
                .map(id -> FlowLaunchEntity.Id.of(id.asString()))
                .collect(Collectors.toSet());
        return find(FlowLaunchEntityForVersion.class, keys).stream()
                .collect(Collectors.toMap(FlowLaunchEntityForVersion::getFlowLaunchId, Function.identity()));
    }


    // *********************************************************************************
    // WARNING - объект не иммутабельный, если его поменять, то сохранить уже не удастся
    // *********************************************************************************
    @Nullable
    @Override
    protected FlowLaunchEntity findInternal(@Nonnull Entity.Id<FlowLaunchEntity> id) {
        var result = super.findInternal(id);
        return result == null ? null : SerializationUtils.copy(result, FlowLaunchEntity.class);
    }


    @Value
    public static class FlowLaunchAccessView implements Table.View {
        @Column(dbType = DbType.UTF8)
        String processId;

        @Column(dbType = DbType.UTF8)
        String triggeredBy;

        @Column(dbType = DbType.JSON, flatten = false)
        LaunchFlowInfo flowInfo;

        @Column(dbType = DbType.JSON, flatten = false)
        @Nonnull
        LaunchVcsInfo vcsInfo;

        public CiProcessId toCiProcessId() {
            return CiProcessId.ofString(processId);
        }
    }


    @Value
    public static class FlowLaunchView implements Table.View {
        @Column(dbType = DbType.UTF8)
        String processId;

        @Column
        int launchNumber;

        @Column(dbType = DbType.UTF8)
        String projectId;

        public LaunchId toLaunchId() {
            return LaunchId.of(CiProcessId.ofString(processId), launchNumber);
        }
    }
}
