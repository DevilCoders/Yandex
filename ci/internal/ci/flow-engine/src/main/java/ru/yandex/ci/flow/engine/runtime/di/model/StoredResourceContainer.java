package ru.yandex.ci.flow.engine.runtime.di.model;

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.stream.Collectors;

import com.google.common.base.Preconditions;
import com.google.common.collect.HashMultimap;
import com.google.common.collect.Multimap;

import ru.yandex.ci.core.job.JobResourceType;
import ru.yandex.ci.core.resources.Resource;

public class StoredResourceContainer {
    private static final StoredResourceContainer EMPTY = new StoredResourceContainer(HashMultimap.create());

    private final Multimap<JobResourceType, StoredResource> resourceMap;
    private final List<StoredResource> resourceList;

    public StoredResourceContainer(Multimap<JobResourceType, StoredResource> resourceMap) {
        this(resourceMap.values());
    }

    public StoredResourceContainer(Collection<StoredResource> resources) {
        this.resourceList = new ArrayList<>(resources);
        this.resourceMap = HashMultimap.create();
        for (StoredResource resource : resources) {
            resourceMap.put(resource.getResourceType(), resource);
        }
    }

    public static StoredResourceContainer empty() {
        return EMPTY;
    }

    public List<StoredResource> getResources() {
        return this.resourceList;
    }

    public boolean containsResource(JobResourceType resourceType) {
        return resourceMap.containsKey(resourceType);
    }

    public Resource instantiate(JobResourceType resourceType) {
        Collection<StoredResource> resourcesByName = resourceMap.get(resourceType);

        Preconditions.checkState(
                resourcesByName.size() > 0,
                "Resource %s not found", resourceType
        );

        Preconditions.checkState(
                resourcesByName.size() == 1,
                "There are %s resources of type %s, expected 1", resourcesByName.size(), resourceType
        );

        return resourcesByName.iterator().next().instantiate();
    }

    public List<Resource> instantiateList(JobResourceType resourceType) {
        Collection<StoredResource> resourcesByName = resourceMap.get(resourceType);

        return resourcesByName.stream()
                .map(StoredResource::instantiate)
                .collect(Collectors.toList());
    }

    public ResourceRefContainer toRefs() {
        return ResourceRefContainer.of(
                resourceList.stream().map(StoredResource::toRef).collect(Collectors.toList())
        );
    }
}
