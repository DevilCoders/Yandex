package ru.yandex.ci.core.tasklet;

import java.util.List;

import com.google.gson.JsonObject;

import ru.yandex.ci.core.job.JobResource;

public interface TaskletMetadataService {
    TaskletMetadata fetchMetadata(TaskletMetadata.Id id);

    List<JobResource> extractResources(TaskletMetadata metadata, SchemaOptions options, JsonObject output);

    TaskletSchema extractSchema(TaskletMetadata metadata, SchemaOptions schemaOptions);
}
