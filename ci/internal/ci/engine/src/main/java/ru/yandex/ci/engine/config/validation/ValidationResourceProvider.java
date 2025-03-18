package ru.yandex.ci.engine.config.validation;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

import com.google.common.base.Strings;
import com.google.common.collect.HashMultimap;
import com.google.common.collect.Multimap;
import lombok.Value;

import ru.yandex.ci.core.job.JobResourceType;
import ru.yandex.ci.core.resources.HasResourceType;
import ru.yandex.ci.flow.engine.AbstractResourceProvider;
import ru.yandex.ci.flow.engine.source_code.model.ConsumedResource;
import ru.yandex.ci.flow.engine.source_code.model.ProducedResource;

public class ValidationResourceProvider extends AbstractResourceProvider<
        ValidationJob,
        List<? extends HasResourceType>,
        Multimap<JobResourceType, ValidationResourceProvider.ResourceParentJobId>> {

    public Multimap<JobResourceType, ResourceParentJobId> getResources(
            ValidationJob job,
            Map<JobResourceType, ConsumedResource> resourceDependencyMap) {
        Multimap<JobResourceType, ResourceParentJobId> actualReceivedResources = HashMultimap.create();

        var staticResources = job.getStaticResources();
        addUpstreamResourcesToResult(job.getUpstreams(),
                filterUpstreamDependencies(resourceDependencyMap, staticResources),
                actualReceivedResources);

        addResourcesToResult(
                resourceDependencyMap,
                staticResources,
                job,
                actualReceivedResources
        );


        addMultiplyResourcesIfRequired(job, actualReceivedResources);

        return actualReceivedResources;
    }

    @Override
    protected List<HasResourceType> getProducedResources(ValidationJob upstreamJob) {
        return upstreamJob.getProducedResources().stream()
                .map(ProducedResource::getResource).collect(Collectors.toList());
    }

    @Override
    protected void addResourcesToResult(
            Map<JobResourceType, ConsumedResource> resourceDependencyMap,
            List<? extends HasResourceType> resources,
            ValidationJob resourceParentJob,
            Multimap<JobResourceType, ResourceParentJobId> actualReceivedResources
    ) {
        resources.stream()
                .filter(r -> resourceDependencyMap.containsKey(r.getResourceType()))
                .forEach(r -> actualReceivedResources.put(
                        r.getResourceType(),
                        new ResourceParentJobId(r, resourceParentJob.getId())
                ));
    }

    private void addMultiplyResourcesIfRequired(
            ValidationJob job,
            Multimap<JobResourceType, ResourceParentJobId> actualReceivedResources) {
        var multiply = job.getJobMultiply();
        if (multiply != null && !Strings.isNullOrEmpty(multiply.getField())) {
            var type = JobResourceType.of(multiply.getField());
            if (!actualReceivedResources.containsKey(type)) {
                // Self-generated resource
                actualReceivedResources.put(type, new ResourceParentJobId(() -> type, job.getId()));
            }
        }
    }

    @SafeVarargs
    private Map<JobResourceType, ConsumedResource> filterUpstreamDependencies(
            Map<JobResourceType, ConsumedResource> resourceDependencyMap,
            List<? extends HasResourceType>... refs) {
        var upstreamResourceDependencyMap = new HashMap<>(resourceDependencyMap);

        // Remove all resource types from upstream if already defined for job
        for (var ref : refs) {
            for (var resource : ref) {
                // Always remove a resource even if this is a list...
                upstreamResourceDependencyMap.remove(resource.getResourceType());
            }
        }
        return upstreamResourceDependencyMap;
    }

    @Value
    public static class ResourceParentJobId {
        HasResourceType resource;
        String jobId;
    }
}
