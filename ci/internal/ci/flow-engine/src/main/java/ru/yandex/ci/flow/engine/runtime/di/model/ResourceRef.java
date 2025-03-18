package ru.yandex.ci.flow.engine.runtime.di.model;

import java.util.UUID;

import javax.annotation.Nonnull;

import lombok.Value;

import ru.yandex.ci.core.job.JobResourceType;
import ru.yandex.ci.core.resources.HasResourceType;
import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value
public class ResourceRef implements HasResourceType {
    @Nonnull
    StoredResourceId id;

    @Nonnull
    JobResourceType resourceType;

    @Nonnull
    UUID sourceCodeId;

    @Override
    public JobResourceType getResourceType() {
        return resourceType;
    }
}
