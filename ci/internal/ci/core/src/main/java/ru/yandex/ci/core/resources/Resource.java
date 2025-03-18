package ru.yandex.ci.core.resources;

import java.beans.Transient;
import java.nio.charset.StandardCharsets;
import java.util.List;
import java.util.UUID;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.google.gson.JsonObject;
import com.google.protobuf.Message;

import ru.yandex.ci.core.common.SourceCodeEntity;
import ru.yandex.ci.core.job.JobResource;
import ru.yandex.ci.core.job.JobResourceType;
import ru.yandex.ci.core.proto.ProtobufSerialization;
import ru.yandex.ci.core.tasklet.SchemaOptions;
import ru.yandex.ci.core.tasklet.TaskletMetadata;

/**
 * Ресурс - единственная реализация, хранящая сериализованное представление Protobuf-ресурса в виде JSON.
 */
public class Resource implements SourceCodeEntity, HasResourceType {

    private final JobResourceType resourceType;
    private final JsonObject data;
    private final String parentField;

    public Resource(JobResourceType resourceType, JsonObject data) {
        this(resourceType, data, null);
    }

    public Resource(JobResourceType resourceType, JsonObject data, @Nullable String parentField) {
        this.resourceType = resourceType;
        this.data = data;
        this.parentField = parentField;
    }

    @Override
    public UUID getSourceCodeId() {
        return uuidFromMessageName(resourceType.getMessageName());
    }

    @Override
    public JobResourceType getResourceType() {
        return resourceType;
    }

    public JsonObject getData() {
        return data;
    }

    /**
     * Возвращает название поля, в котором лежали данные ресурса в a.yaml input.
     * Нужно для разделения ресурсов на статические из a.yaml и полученные по зависимостям от других тасок
     *
     * @return название поля или null, если ресурс получен из других тасок.
     * @see ru.yandex.ci.core.tasklet.SchemaService#composeInput(TaskletMetadata, SchemaOptions, List)
     */
    @Nullable
    public String getParentField() {
        return parentField;
    }

    @Transient
    @JsonIgnore
    public String renderTitle() {
        return resourceType.getMessageName();
    }

    @Transient
    @JsonIgnore
    public String renderResource() {
        return data.toString();
    }

    public static Resource of(Message message) {
        return of(message, null);
    }

    public static Resource of(Message message, @Nullable String parentField) {
        return new Resource(
                JobResourceType.ofMessage(message),
                ProtobufSerialization.serializeToGson(message),
                parentField
        );
    }

    public static Resource of(JobResource jobResource) {
        return new Resource(jobResource.getResourceType(), jobResource.getData(), jobResource.getParentField());
    }

    public static UUID uuidFromMessageName(String messageTypeName) {
        return UUID.nameUUIDFromBytes(messageTypeName.getBytes(StandardCharsets.UTF_8));
    }

}
