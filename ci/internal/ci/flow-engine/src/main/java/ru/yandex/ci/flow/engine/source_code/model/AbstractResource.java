package ru.yandex.ci.flow.engine.source_code.model;

import java.util.UUID;

import com.google.common.base.Preconditions;

public class AbstractResource {

    private final ResourceObject resource;
    private final boolean isList;

    public AbstractResource(ResourceObject resource, boolean isList) {
        Preconditions.checkNotNull(resource);

        this.resource = resource;
        this.isList = isList;
    }

    public ResourceObject getResource() {
        return resource;
    }

    public boolean isList() {
        return isList;
    }

    public UUID getId() {
        return resource.getId();
    }
}
