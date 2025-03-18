package ru.yandex.ci.flow.engine.source_code.model;

import javax.annotation.Nullable;

import com.google.protobuf.Descriptors;

public class ConsumedResource extends AbstractResource {

    @Nullable
    private final String field;
    @Nullable
    private final Descriptors.Descriptor descriptor;

    public ConsumedResource(
            @Nullable String field,
            @Nullable Descriptors.Descriptor descriptor,
            ResourceObject resource,
            boolean isList
    ) {
        super(resource, isList);
        this.field = field;
        this.descriptor = descriptor;
    }

    @Nullable
    public String getField() {
        return field;
    }

    @Nullable
    public Descriptors.Descriptor getDescriptor() {
        return descriptor;
    }

    public static ConsumedResource taskletResource(ResourceObject resource, boolean isList) {
        return new ConsumedResource(null, null, resource, isList);
    }
}
