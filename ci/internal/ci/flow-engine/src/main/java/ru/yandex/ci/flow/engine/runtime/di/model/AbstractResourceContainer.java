package ru.yandex.ci.flow.engine.runtime.di.model;

import java.util.List;
import java.util.Map;

import ru.yandex.ci.core.job.JobResourceType;
import ru.yandex.ci.core.resources.Resource;

public interface AbstractResourceContainer {
    boolean containsOfType(JobResourceType resourceType);

    List<Resource> getAll();

    Map<StoredResourceId, Resource> getAllById();

    List<Resource> getOfType(JobResourceType type);

    Resource getSingleOfType(JobResourceType type);

}
