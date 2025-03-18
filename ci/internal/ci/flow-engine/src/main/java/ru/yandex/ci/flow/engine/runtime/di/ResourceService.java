package ru.yandex.ci.flow.engine.runtime.di;

import java.time.Clock;
import java.util.List;

import javax.annotation.Nonnull;

import com.google.common.collect.HashMultimap;
import com.google.common.collect.Multimap;
import lombok.RequiredArgsConstructor;

import ru.yandex.ci.core.job.JobResourceType;
import ru.yandex.ci.core.resources.Resource;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.runtime.di.model.AbstractResourceContainer;
import ru.yandex.ci.flow.engine.runtime.di.model.ResourceRefContainer;
import ru.yandex.ci.flow.engine.runtime.di.model.StoredResource;
import ru.yandex.ci.flow.engine.runtime.di.model.StoredResourceContainer;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchRef;

@RequiredArgsConstructor
public class ResourceService {

    @Nonnull
    private final CiDb db;

    @Nonnull
    private final Clock clock;

    /**
     * Создаёт {@link StoredResourceContainer} по заданному списку ресурсов и сохраняет его в персистентное хранилище.
     */
    public StoredResourceContainer saveResources(List<? extends Resource> resources,
                                                 FlowLaunchRef launchRef,
                                                 ResourceEntity.ResourceClass resourceClass) {
        if (resources.isEmpty()) {
            return StoredResourceContainer.empty();
        }

        Multimap<JobResourceType, StoredResource> resourceMultimap = HashMultimap.create();

        for (Resource manualResource : resources) {
            resourceMultimap.put(
                    manualResource.getResourceType(),
                    launchRef.fromResource(manualResource)
            );
        }

        var storedResourceContainer = new StoredResourceContainer(resourceMultimap);
        db.currentOrTx(() ->
                db.resources().saveResources(storedResourceContainer, resourceClass, clock));
        return storedResourceContainer;
    }

    public AbstractResourceContainer loadResources(ResourceRefContainer resourceRefs) {
        var container = db.currentOrReadOnly(() ->
                db.resources().loadResources(resourceRefs));
        return new StoredResourceContainerWrapper(container);
    }

}
