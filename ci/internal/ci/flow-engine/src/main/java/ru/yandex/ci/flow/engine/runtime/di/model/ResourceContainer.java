package ru.yandex.ci.flow.engine.runtime.di.model;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.Optional;

import com.google.common.base.Preconditions;
import com.google.common.collect.LinkedHashMultimap;
import com.google.common.collect.Multimap;

import ru.yandex.ci.core.job.JobResourceType;
import ru.yandex.ci.core.resources.Resource;

public class ResourceContainer implements AbstractResourceContainer {

    private final Multimap<JobResourceType, Resource> resourceMap;
    private final List<Resource> resourceList;

    public ResourceContainer(Resource... resources) {
        this(Arrays.asList(resources));
    }

    public ResourceContainer(Collection<Resource> resources) {
        this.resourceList = new ArrayList<>(resources);

        this.resourceMap = LinkedHashMultimap.create();
        for (Resource resource : resources) {
            this.resourceMap.put(resource.getResourceType(), resource);
        }
    }

    public ResourceContainer(Multimap<String, Resource> resourceMap) {
        this(resourceMap.values());
    }

    @Override
    public boolean containsOfType(JobResourceType resourceType) {
        return resourceMap.containsKey(resourceType);
    }

    @Override
    public List<Resource> getOfType(JobResourceType resourceType) {
        return new ArrayList<>(resourceMap.get(resourceType));
    }

    @Override
    public Resource getSingleOfType(JobResourceType resourceType) {
        return getOptionalSingleOfType(resourceType)
                .orElseThrow(() -> new IllegalStateException(String.format("Resource %s not found", resourceType)));
    }

    public Optional<Resource> getOptionalSingleOfType(JobResourceType resourceType) {
        Collection<Resource> resources = resourceMap.get(resourceType);

        if (resources.isEmpty()) {
            return Optional.empty();
        }

        Preconditions.checkState(
                resources.size() == 1,
                "There are %s resources of type %s, expected 1", resources.size(), resourceType
        );

        return Optional.of(resources.iterator().next());
    }

    @Override
    public List<Resource> getAll() {
        return Collections.unmodifiableList(resourceList);
    }

    @Override
    public Map<StoredResourceId, Resource> getAllById() {
        throw new UnsupportedOperationException();
    }

    @Override
    public String toString() {
        return "ResourceContainer{" +
                ", resourceList=" + resourceList +
                '}';
    }
}
