package ru.yandex.ci.flow.engine.runtime.state.calculator;

import java.util.HashMap;
import java.util.Map;

import com.google.common.base.Preconditions;
import org.apache.commons.collections4.CollectionUtils;

import ru.yandex.ci.core.job.JobResourceType;
import ru.yandex.ci.core.resources.Resource;
import ru.yandex.ci.flow.engine.AbstractResourceProvider;
import ru.yandex.ci.flow.engine.definition.builder.JobBuilder;
import ru.yandex.ci.flow.engine.runtime.di.model.ResourceRef;
import ru.yandex.ci.flow.engine.runtime.di.model.ResourceRefContainer;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.JobState;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChange;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChangeType;
import ru.yandex.ci.flow.engine.source_code.model.ConsumedResource;

public class ResourceProvider
        extends AbstractResourceProvider<JobRuntime, ResourceRefContainer, ResourceRefContainerBuilder> {

    /**
     * Проходится BFS по апстримам и собирает нужные ресурсы c:
     * <ul>
     * <li>со всех апстримов (непосредственных и транизитивных) для данной джобы {@link JobRuntime};</li>
     * <li>ресурсов, переданных при описании флоу методом
     * {@link JobBuilder#withResources(Resource...)};</li>
     * <li>ресурсов, заданных при старте флоу.</li>
     * </ul>
     */
    ResourceRefContainer getResources(JobRuntime jobRuntime, FlowLaunchEntity flowLaunch) {
        JobState jobState = jobRuntime.getJobState();

        var executor = jobRuntime.getExecutorObject();
        Preconditions.checkState(executor != null, "Executor cannot be null at this point");

        var resourceDependencyMap = executor.getResourceDependencyMap();

        var resultBuilder = ResourceRefContainerBuilder.create();
        addUpstreamResourcesToResult(
                jobRuntime.getUpstreams(),
                filterUpstreamDependencies(
                        jobRuntime,
                        resourceDependencyMap,
                        flowLaunch.getManualResources(),
                        jobState.getStaticResources()
                ),
                resultBuilder);

        addResourcesToResult(resourceDependencyMap, flowLaunch.getManualResources(), jobRuntime, resultBuilder);

        addResourcesToResult(resourceDependencyMap, jobState.getStaticResources(), jobRuntime, resultBuilder);

        return resultBuilder.build();
    }

    @Override
    protected ResourceRefContainer getProducedResources(JobRuntime upstreamJob) {
        var delegatedResource = upstreamJob.getJobState().getDelegatedOutputResources();
        if (delegatedResource != null) {
            return delegatedResource.getResourceRef();
        }

        JobStatus status = upstreamJob.getStatus();
        if (status != null) {
            StatusChange statusChange = status.getStatusChange();
            if (statusChange == null || !statusChange.getType().equals(StatusChangeType.SUCCESSFUL)) {
                return ResourceRefContainer.empty();
            }
        }

        return upstreamJob.getLastLaunch().getProducedResources() == null
                ? ResourceRefContainer.empty()
                : upstreamJob.getLastLaunch().getProducedResources();
    }


    @Override
    protected void addResourcesToResult(
            Map<JobResourceType, ConsumedResource> resourceDependencyMap,
            ResourceRefContainer resources,
            JobRuntime resourceParentJob,
            ResourceRefContainerBuilder resultBuilder
    ) {
        for (ResourceRef resourceRef : resources.getResources()) {
            var dependantType = resourceRef.getResourceType();
            var resource = resourceDependencyMap.get(dependantType);
            if (resource != null) {
                if (resource.isList()) {
                    resultBuilder.addListResource(dependantType, resourceRef);
                } else {
                    resultBuilder.addSingularResource(dependantType, resourceRef);
                }
            }
        }
    }

    private Map<JobResourceType, ConsumedResource> filterUpstreamDependencies(
            JobRuntime jobRuntime,
            Map<JobResourceType, ConsumedResource> resourceDependencyMap,
            ResourceRefContainer... refs) {
        var upstreamResourceDependencyMap = new HashMap<>(resourceDependencyMap);

        var skipResources = jobRuntime.getJobState().getSkipUpstreamResources();
        if (CollectionUtils.isNotEmpty(skipResources)) {
            upstreamResourceDependencyMap.keySet().removeAll(skipResources);
        }

        // Remove all resource types from upstream if already defined for job
        for (var ref : refs) {
            for (var resource : ref.getResources()) {
                // Always remove a resource even if this is a list...
                upstreamResourceDependencyMap.remove(resource.getResourceType());
            }
        }

        return upstreamResourceDependencyMap;
    }
}
