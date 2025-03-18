package ru.yandex.ci.core.taskletv2;

import java.util.List;

import com.google.gson.JsonObject;
import com.google.protobuf.ByteString;
import com.google.protobuf.Message;

import ru.yandex.ci.core.job.JobResource;
import ru.yandex.ci.core.tasklet.SchemaOptions;
import ru.yandex.ci.core.tasklet.TaskletSchema;
import ru.yandex.ci.util.Clearable;

public interface TaskletV2MetadataService extends Clearable {
    LocalCacheService getLocalCache();

    TaskletV2Metadata fetchMetadata(TaskletV2Metadata.Id id);

    TaskletSchema extractSchema(TaskletV2Metadata metadata, SchemaOptions schemaOptions);

    Message composeInput(TaskletV2Metadata metadata, SchemaOptions options, List<JobResource> inputs);

    JsonObject buildOutput(TaskletV2Metadata metadata, ByteString serializedOutput);

    List<JobResource> extractResources(TaskletV2Metadata metadata, SchemaOptions options, JsonObject output);

    interface LocalCacheService {
        TaskletV2Metadata fetchMetadata(TaskletV2Metadata.Description key);
    }
}
