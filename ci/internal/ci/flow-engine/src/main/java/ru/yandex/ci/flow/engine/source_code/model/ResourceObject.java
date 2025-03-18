package ru.yandex.ci.flow.engine.source_code.model;

import java.util.Objects;
import java.util.UUID;

import ru.yandex.ci.core.job.JobResourceType;
import ru.yandex.ci.core.resources.HasResourceType;
import ru.yandex.ci.core.resources.Resource;

public class ResourceObject extends SourceCodeObject<Resource> implements HasResourceType {
    private final boolean editable;

    private final String title;
    private final String description;
    private final JobResourceType jobResourceType;

    public ResourceObject(
            UUID id,
            Class<? extends Resource> clazz,
            boolean editable,
            String title,
            String description,
            JobResourceType jobResourceType
    ) {
        super(id, clazz, SourceCodeObjectType.RESOURCE);
        this.editable = editable;
        this.title = title;
        this.description = description;
        this.jobResourceType = jobResourceType;
    }

    public boolean isEditable() {
        return editable;
    }

    public String getTitle() {
        return title;
    }

    public String getDescription() {
        return description;
    }

    @Override
    public JobResourceType getResourceType() {
        return jobResourceType;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) {
            return true;
        }

        if (!(o instanceof ResourceObject)) {
            return false;
        }

        ResourceObject that = (ResourceObject) o;
        return Objects.equals(id, that.id);
    }

    @Override
    public int hashCode() {
        return Objects.hash(id);
    }
}
