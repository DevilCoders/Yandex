package ru.yandex.ci.flow.engine.runtime.di;

import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

import ru.yandex.ci.core.job.JobResourceType;
import ru.yandex.ci.core.resources.Resource;
import ru.yandex.ci.flow.engine.runtime.di.model.AbstractResourceContainer;
import ru.yandex.ci.flow.engine.runtime.di.model.StoredResource;
import ru.yandex.ci.flow.engine.runtime.di.model.StoredResourceContainer;
import ru.yandex.ci.flow.engine.runtime.di.model.StoredResourceId;

class StoredResourceContainerWrapper implements AbstractResourceContainer {
    private final StoredResourceContainer storedResourceContainer;

    StoredResourceContainerWrapper(StoredResourceContainer storedResourceContainer) {
        this.storedResourceContainer = storedResourceContainer;
    }

    @Override
    public boolean containsOfType(JobResourceType resourceType) {
        return storedResourceContainer.containsResource(resourceType);
    }

    @Override
    public List<Resource> getAll() {
        return storedResourceContainer.getResources().stream()
                .map(StoredResource::instantiate)
                .collect(Collectors.toList());
    }

    @Override
    public Map<StoredResourceId, Resource> getAllById() {
        return storedResourceContainer.getResources().stream()
                .collect(Collectors.toMap(
                        StoredResource::getId,
                        StoredResource::instantiate));
    }

    @Override
    public List<Resource> getOfType(JobResourceType resourceType) {
        return storedResourceContainer.instantiateList(resourceType);
    }

    @Override
    public Resource getSingleOfType(JobResourceType resourceType) {
        return storedResourceContainer.instantiate(resourceType);
    }
}
