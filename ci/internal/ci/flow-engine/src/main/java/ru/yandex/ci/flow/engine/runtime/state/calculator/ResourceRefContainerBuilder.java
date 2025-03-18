package ru.yandex.ci.flow.engine.runtime.state.calculator;

import com.google.common.base.Preconditions;
import com.google.common.collect.HashMultimap;
import com.google.common.collect.Multimap;

import ru.yandex.ci.core.job.JobResourceType;
import ru.yandex.ci.flow.engine.runtime.di.model.ResourceRef;
import ru.yandex.ci.flow.engine.runtime.di.model.ResourceRefContainer;

class ResourceRefContainerBuilder {
    private final Multimap<JobResourceType, ResourceRef> resourceMap = HashMultimap.create();

    private ResourceRefContainerBuilder() {
    }

    public static ResourceRefContainerBuilder create() {
        return new ResourceRefContainerBuilder();
    }

    public ResourceRefContainerBuilder addSingularResource(JobResourceType type, ResourceRef resourceRef) {
        Preconditions.checkState(
                !resourceMap.containsKey(type),
                "Resource '%s' already added",
                resourceRef.getResourceType()
        );

        resourceMap.put(type, resourceRef);
        return this;
    }

    public ResourceRefContainerBuilder addListResource(JobResourceType type, ResourceRef resourceRef) {
        resourceMap.put(type, resourceRef);
        return this;
    }

    boolean containsResourceOf(JobResourceType type) {
        return resourceMap.containsKey(type);
    }

    public ResourceRefContainer build() {
        return ResourceRefContainer.of(resourceMap.values());
    }
}
