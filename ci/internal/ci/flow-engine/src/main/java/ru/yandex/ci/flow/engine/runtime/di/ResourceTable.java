package ru.yandex.ci.flow.engine.runtime.di;

import java.time.Clock;
import java.util.UUID;
import java.util.stream.Collectors;

import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;
import yandex.cloud.repository.kikimr.yql.YqlView;

import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.core.job.JobResourceType;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.di.ResourceEntity.ResourceClass;
import ru.yandex.ci.flow.engine.runtime.di.model.ResourceRef;
import ru.yandex.ci.flow.engine.runtime.di.model.ResourceRefContainer;
import ru.yandex.ci.flow.engine.runtime.di.model.StoredResource;
import ru.yandex.ci.flow.engine.runtime.di.model.StoredResourceContainer;
import ru.yandex.ci.flow.engine.runtime.di.model.StoredResourceId;

public class ResourceTable extends KikimrTableCi<ResourceEntity> {

    public ResourceTable(QueryExecutor executor) {
        super(ResourceEntity.class, executor);
    }

    public StoredResourceContainer loadResources(ResourceRefContainer resourceRefs) {
        var resourceIds = resourceRefs.getResources().stream()
                .map(ResourceRef::getId)
                .map(StoredResourceId::asString)
                .map(ResourceEntity.Id::of)
                .collect(Collectors.toSet());

        var resourceList = find(resourceIds);

        return new StoredResourceContainer(resourceList.stream()
                .map(ResourceTable::toStoredResource)
                .collect(Collectors.toList()));
    }

    public int getResourceCount(FlowLaunchId flowLaunchId, ResourceClass resourceClass) {
        return (int) count(
                YqlPredicate.where("flowLaunchId").eq(flowLaunchId.asString()),
                YqlPredicate.where("resourceClass").eq(resourceClass),
                YqlView.index(ResourceEntity.IDX_FLOW_LAUNCH_ID));
    }

    public void saveResources(StoredResourceContainer resources, ResourceClass resourceClass, Clock clock) {
        for (StoredResource resource : resources.getResources()) {
            var entity = ResourceEntity.builder()
                    .id(ResourceEntity.Id.of(resource.getId().asString()))
                    .flowId(resource.getFlowId())
                    .flowLaunchId(resource.getFlowLaunchId().asString())
                    .modifications(resource.getModifications())
                    .resourceType(resource.getResourceType().getMessageName())
                    .classBased(false)
                    .sourceCodeId(resource.getSourceCodeId().toString())
                    .object(resource.getObject())
                    .resourceClass(resourceClass)
                    .updated(clock.instant())
                    .build();
            save(entity);
        }
    }

    //
    private static StoredResource toStoredResource(ResourceEntity entity) {
        return new StoredResource(
                StoredResourceId.of(entity.getId().getId()),
                entity.getFlowId(),
                FlowLaunchId.of(entity.getFlowLaunchId()),
                UUID.fromString(entity.getSourceCodeId()),
                entity.getObject(),
                JobResourceType.of(entity.getResourceType()),
                entity.getModifications());
    }
}
