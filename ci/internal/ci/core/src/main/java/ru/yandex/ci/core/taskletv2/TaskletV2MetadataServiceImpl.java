package ru.yandex.ci.core.taskletv2;

import java.time.Duration;
import java.util.HashMap;
import java.util.List;

import javax.annotation.Nullable;

import com.google.common.cache.CacheBuilder;
import com.google.common.cache.CacheLoader;
import com.google.common.cache.LoadingCache;
import com.google.gson.JsonObject;
import com.google.protobuf.ByteString;
import com.google.protobuf.Message;
import com.google.protobuf.TextFormat;
import io.micrometer.core.instrument.MeterRegistry;
import io.micrometer.core.instrument.binder.cache.GuavaCacheMetrics;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.client.taskletv2.TaskletV2Client;
import ru.yandex.ci.common.proto.ProtoConverter;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.job.JobResource;
import ru.yandex.ci.core.tasklet.DescriptorsParser;
import ru.yandex.ci.core.tasklet.SchemaOptions;
import ru.yandex.ci.core.tasklet.SchemaService;
import ru.yandex.ci.core.tasklet.TaskletMetadataValidationException;
import ru.yandex.ci.core.tasklet.TaskletSchema;
import ru.yandex.ci.util.ExceptionUtils;
import ru.yandex.tasklet.api.v2.SchemaRegistryServiceOuterClass.GetSchemaRequest;
import ru.yandex.tasklet.api.v2.TaskletServiceOuterClass.GetBuildRequest;
import ru.yandex.tasklet.api.v2.TaskletServiceOuterClass.GetLabelRequest;

@Slf4j
public class TaskletV2MetadataServiceImpl implements TaskletV2MetadataService {

    private static final Duration TASKLET_METADATA_CACHE_EXPIRATION = Duration.ofDays(1);
    private static final int TASKLET_METADATA_CACHE_SIZE = 10_000;

    private final TaskletV2Client taskletV2Client;
    private final SchemaService schemaService;
    private final CiMainDb db;

    private final LoadingCache<TaskletV2Metadata.Id, TaskletV2Metadata> taskletBuildCache;

    public TaskletV2MetadataServiceImpl(
            TaskletV2Client taskletV2Client,
            SchemaService schemaService,
            CiMainDb db,
            @Nullable MeterRegistry meterRegistry
    ) {
        this.taskletV2Client = taskletV2Client;
        this.schemaService = schemaService;
        this.db = db;
        this.taskletBuildCache = CacheBuilder.newBuilder()
                .expireAfterAccess(TASKLET_METADATA_CACHE_EXPIRATION)
                .maximumSize(TASKLET_METADATA_CACHE_SIZE)
                .recordStats()
                .build(CacheLoader.from(this::fetchBuild));
        if (meterRegistry != null) {
            GuavaCacheMetrics.monitor(meterRegistry, taskletBuildCache, "tasklet-v2-build");
        }
    }

    @Override
    public void clear() {
        this.taskletBuildCache.invalidateAll();
    }

    @Override
    public LocalCacheService getLocalCache() {
        // reuse builds during single a.yaml parse
        var localCache = new HashMap<TaskletV2Metadata.Description, TaskletV2Metadata>();
        return key -> localCache.computeIfAbsent(key, TaskletV2MetadataServiceImpl.this::fetchMetadataByLookup);
    }

    private TaskletV2Metadata fetchMetadataByLookup(TaskletV2Metadata.Description key) {
        log.info("Loading tasklet metadata for {}", key);

        var labelRequest = GetLabelRequest.newBuilder()
                .setNamespace(key.getNamespace())
                .setTasklet(key.getTasklet())
                .setLabel(key.getLabel())
                .build();
        var label = taskletV2Client.getLabel(labelRequest).getLabel();

        var buildId = label.getSpec().getBuildId();
        if (buildId.isEmpty()) {
            throw new TaskletMetadataValidationException("Tasklet V2 label %s has no build"
                    .formatted(TextFormat.shortDebugString(labelRequest)));
        }

        return fetchMetadata(TaskletV2Metadata.Id.of(buildId));
    }

    @Override
    public TaskletV2Metadata fetchMetadata(TaskletV2Metadata.Id id) {
        try {
            return taskletBuildCache.get(id);
        } catch (Exception e) {
            throw ExceptionUtils.unwrap(e);
        }
    }

    @Override
    public List<JobResource> extractResources(TaskletV2Metadata metadata, SchemaOptions options, JsonObject output) {
        return schemaService.extractOutput(metadata, options, output);
    }

    @Override
    public TaskletSchema extractSchema(TaskletV2Metadata metadata, SchemaOptions schemaOptions) {
        return schemaService.extractSchema(metadata, schemaOptions);
    }

    @Override
    public Message composeInput(TaskletV2Metadata metadata, SchemaOptions options, List<JobResource> inputs) {
        // TODO: optimize?
        var input = schemaService.composeInput(metadata, options, inputs);
        return schemaService.transformInputToProto(metadata, input);
    }

    @Override
    public JsonObject buildOutput(TaskletV2Metadata metadata, ByteString serializedOutput) {
        // TODO: optimize?
        return schemaService.transformOutputToJson(metadata, serializedOutput);
    }

    private TaskletV2Metadata fetchBuild(TaskletV2Metadata.Id id) {
        return db.currentOrReadOnly(() -> db.taskletV2Metadata().find(id))
                .orElseGet(() -> {
                    var build = fetchBuildImpl(id);
                    db.tx(() -> db.taskletV2Metadata().save(build));
                    return build;
                });
    }

    private TaskletV2Metadata fetchBuildImpl(TaskletV2Metadata.Id id) {
        log.info("Loading tasklet v2 metadata for {}", id);
        var buildRequest = GetBuildRequest.newBuilder()
                .setBuildId(id.getId())
                .build();
        var build = taskletV2Client.getBuild(buildRequest).getBuild();

        var schemaProto = build.getSpec().getSchema().getSimpleProto();
        var schemaRequest = GetSchemaRequest.newBuilder()
                .setHash(schemaProto.getSchemaHash())
                .build();
        var schema = taskletV2Client.getSchema(schemaRequest);

        var inputMessage = schemaProto.getInputMessage();
        var outputMessage = schemaProto.getOutputMessage();

        var optimizedDescriptors = DescriptorsParser.optimizeDescriptors(
                schema.getSchema(),
                List.of(inputMessage, outputMessage)
        );
        schemaService.validate(inputMessage, outputMessage, optimizedDescriptors);

        var metadata = TaskletV2Metadata.builder()
                .id(id)
                .created(ProtoConverter.convert(build.getMeta().getCreatedAt()))
                .revision(build.getMeta().getRevision())
                .descriptors(optimizedDescriptors.toByteArray())
                .inputMessage(inputMessage)
                .outputMessage(outputMessage)
                .build();

        log.info("Loaded tasklet v2 metadata: {}", metadata);

        return metadata;
    }
}
