package ru.yandex.ci.flow.engine.runtime.di.model;

import java.util.Collection;
import java.util.List;

import javax.annotation.Nonnull;

import lombok.Value;

import ru.yandex.ci.util.CollectionUtils;
import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value(staticConstructor = "of")
public class ResourceRefContainer {
    private static final ResourceRefContainer EMPTY = ResourceRefContainer.of();

    @Nonnull
    List<ResourceRef> resources;

    public ResourceRefContainer mergeWith(ResourceRefContainer container) {
        return of(CollectionUtils.join(resources, container.getResources()));
    }

    public static ResourceRefContainer of(Collection<ResourceRef> resourceRefs) {
        return new ResourceRefContainer(List.copyOf(resourceRefs));
    }

    public static ResourceRefContainer of(ResourceRef... resourceRefs) {
        return new ResourceRefContainer(List.of(resourceRefs));
    }

    public static ResourceRefContainer empty() {
        return EMPTY;
    }
}
