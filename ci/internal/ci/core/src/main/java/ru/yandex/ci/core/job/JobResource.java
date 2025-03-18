package ru.yandex.ci.core.job;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.annotations.VisibleForTesting;
import com.google.gson.JsonObject;
import com.google.protobuf.GeneratedMessageV3;
import lombok.Value;
import lombok.With;

import ru.yandex.ci.core.proto.ProtobufSerialization;
import ru.yandex.ci.core.resources.Resource;
import ru.yandex.ci.ydb.Persisted;

// TODO заменить на ProtobufResource
@Persisted
@Value
@SuppressWarnings("ReferenceEquality")
public class JobResource {
    @Nonnull
    JobResourceType resourceType;

    @With
    @Nonnull
    JsonObject data;

    @With
    @Nullable
    transient String parentField;

    boolean optional;

    public static JobResource withParentField(JobResourceType resourceType, JsonObject data, String parentField) {
        return new JobResource(resourceType, data, parentField, false);
    }

    public static JobResource optional(JobResourceType resourceType, JsonObject data) {
        return new JobResource(resourceType, data, null, true);
    }

    public static JobResource mandatory(JobResourceType resourceType, JsonObject data) {
        return new JobResource(resourceType, data, null, false);
    }

    public static JobResource regular(GeneratedMessageV3 message) {
        return mandatory(JobResourceType.ofMessage(message), serializeMessage(message));
    }

    public static JobResource optional(GeneratedMessageV3 message) {
        return optional(JobResourceType.ofMessage(message), serializeMessage(message));
    }

    public static JobResource of(Resource resource) {
        return withParentField(resource.getResourceType(), resource.getData(), resource.getParentField());
    }

    private static JsonObject serializeMessage(GeneratedMessageV3 message) {
        return ProtobufSerialization.serializeToGson(message);
    }

    @VisibleForTesting
    public static JobResource any(String messageName, JsonObject data) {
        return new JobResource(JobResourceType.of(messageName), data, null, false);
    }
}
