package ru.yandex.ci.core.tasklet;

import java.io.ByteArrayOutputStream;
import java.time.Duration;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.TreeSet;
import java.util.zip.DataFormatException;
import java.util.zip.Inflater;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.PropertyNamingStrategies;
import com.google.common.cache.CacheBuilder;
import com.google.common.cache.CacheLoader;
import com.google.common.cache.LoadingCache;
import com.google.common.io.BaseEncoding;
import com.google.gson.JsonObject;
import io.micrometer.core.instrument.MeterRegistry;
import io.micrometer.core.instrument.binder.cache.GuavaCacheMetrics;
import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Value;
import lombok.With;
import lombok.extern.slf4j.Slf4j;
import org.apache.commons.lang3.StringUtils;

import ru.yandex.ci.client.base.http.HttpException;
import ru.yandex.ci.client.sandbox.SandboxClient;
import ru.yandex.ci.client.sandbox.api.ResourceInfo;
import ru.yandex.ci.client.sandbox.api.ResourceState;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.job.JobResource;
import ru.yandex.ci.util.ExceptionUtils;

@Slf4j
public class TaskletMetadataServiceImpl implements TaskletMetadataService {

    private static final String SANDBOX_TASKS_BINARY_TYPE = "SANDBOX_TASKS_BINARY";
    private static final String SCHEMA_ATTRIBUTE_PREFIX = "schema_";

    private static final Duration TASKLET_METADATA_CACHE_EXPIRATION = Duration.ofDays(1);
    private static final int TASKLET_METADATA_CACHE_SIZE = 10_000;

    private final SandboxClient sandboxClient;
    private final CiMainDb db;
    private final SchemaService schemaService;
    private final LoadingCache<TaskletMetadata.Id, TaskletMetadata> taskletMetadataCache;

    public TaskletMetadataServiceImpl(@Nonnull SandboxClient sandboxClient,
                                      @Nonnull CiMainDb db,
                                      @Nonnull SchemaService schemaService,
                                      @Nullable MeterRegistry meterRegistry) {
        this.sandboxClient = sandboxClient;
        this.db = db;
        this.schemaService = schemaService;

        taskletMetadataCache = CacheBuilder.newBuilder()
                .expireAfterAccess(TASKLET_METADATA_CACHE_EXPIRATION)
                .maximumSize(TASKLET_METADATA_CACHE_SIZE)
                .recordStats()
                .build(CacheLoader.from(this::fetchMetadataImpl));
        if (meterRegistry != null) {
            GuavaCacheMetrics.monitor(meterRegistry, taskletMetadataCache, "tasklet-metadata");
        }
    }


    @Override
    public TaskletMetadata fetchMetadata(TaskletMetadata.Id id) {
        try {
            return taskletMetadataCache.get(id);
        } catch (Exception e) {
            throw ExceptionUtils.unwrap(e);
        }
    }

    private TaskletMetadata fetchMetadataImpl(TaskletMetadata.Id id) {
        return db.currentOrReadOnly(() -> db.taskletMetadata().find(id))
                .orElseGet(() -> {
                    var fetched = fetch(id);
                    db.tx(() -> db.taskletMetadata().save(fetched));
                    return fetched;
                });
    }

    private TaskletMetadata fetch(TaskletMetadata.Id id) {
        switch (id.getRuntime()) {
            case SANDBOX -> {
                if (id.getSandboxResourceId() <= 0) {
                    throw new TaskletMetadataValidationException(
                            "sandbox tasklet should have sandbox resource id: " + id
                    );
                }
                return fetchFromSandbox(id.getSandboxResourceId(), id.getImplementation());
            }
            default -> throw new TaskletMetadataValidationException(String.format(
                    "unexpected runtime %s: %s", id.getRuntime(), id
            ));
        }
    }

    public TaskletMetadata fetchFromSandbox(long sandboxResourceId, String taskletImplName) {
        ResourceInfo resourceInfo;
        try {
            resourceInfo = sandboxClient.getResourceInfo(sandboxResourceId);
        } catch (HttpException ex) {
            if (ex.getHttpCode() != 404) {
                throw ex;
            }
            throw new TaskletMetadataValidationException(String.format(
                    "sandbox resource %d is not found for tasklet %s", sandboxResourceId, taskletImplName
            ));
        }

        if (resourceInfo == null) {
            throw new TaskletMetadataValidationException(String.format(
                    "resource %d cannot be loaded for tasklet %s",
                    sandboxResourceId, taskletImplName
            ));
        }

        if (resourceInfo.getState() != ResourceState.READY) {
            throw new TaskletMetadataValidationException(String.format(
                    "resource %d is not ready for tasklet %s. State is %s",
                    sandboxResourceId, taskletImplName, resourceInfo.getState()
            ));
        }

        if (!SANDBOX_TASKS_BINARY_TYPE.equals(resourceInfo.getType())) {
            throw new TaskletMetadataValidationException(String.format(
                    "resource %d is not a tasklet binary for tasklet %s. Type is %s",
                    sandboxResourceId, taskletImplName, resourceInfo.getType()
            ));
        }

        var arch = resourceInfo.getArch();
        if (!StringUtils.isEmpty(arch) && !arch.equalsIgnoreCase("linux")) {
            throw new TaskletMetadataValidationException(String.format(
                    "resource %d is compiled for arch %s. All Tasklets V1 must be compiled for Linux",
                    sandboxResourceId, arch
            ));
        }

        SchemaAttribute schema = lookupSchema(sandboxResourceId, taskletImplName, resourceInfo.getAttributes());

        byte[] descriptors = decompressDescriptors(schema.getMeta(), sandboxResourceId, taskletImplName);
        schemaService.validate(schema.getInput(), schema.getOutput(), descriptors);

        return new TaskletMetadata.Builder()
                .id(TaskletMetadata.Id.of(taskletImplName, TaskletRuntime.SANDBOX, sandboxResourceId))
                .name(schema.getName())
                .sandboxTask(schema.getSbTask())
                .inputTypeString(schema.getInput())
                .outputTypeString(schema.getOutput())
                .descriptors(descriptors)
                .features(Objects.requireNonNullElseGet(schema.getFeatures(), Features::empty))
                .build();
    }

    private static byte[] decompressDescriptors(String meta, long sandboxResourceId, String taskletImplName) {
        try {
            byte[] bytes = BaseEncoding.base64().decode(meta);
            Inflater inflater = new Inflater();
            inflater.setInput(bytes);

            ByteArrayOutputStream baos = new ByteArrayOutputStream();
            byte[] buffer = new byte[1024];
            while (!inflater.finished()) {
                int read = inflater.inflate(buffer);
                baos.write(buffer, 0, read);
            }
            return baos.toByteArray();

        } catch (DataFormatException e) {
            throw new TaskletMetadataValidationException(
                    String.format(
                            "error when decompressing schema meta (sandbox resource %d, tasklet %s)",
                            sandboxResourceId, taskletImplName
                    ),
                    e
            );
        }
    }

    private static SchemaAttribute lookupSchema(
            long sandboxResourceId,
            String taskletImplName,
            Map<String, String> attributes
    ) {
        var checkedImplementations = new TreeSet<String>();
        for (var entry : attributes.entrySet()) {
            if (!entry.getKey().startsWith(SCHEMA_ATTRIBUTE_PREFIX)) {
                continue;
            }

            try {
                ObjectMapper mapper = new ObjectMapper();
                mapper.setPropertyNamingStrategy(PropertyNamingStrategies.LOWER_CAMEL_CASE);
                SchemaAttribute attribute = mapper.readValue(entry.getValue(), SchemaAttribute.class);
                checkedImplementations.addAll(attribute.getImplementations());
                if (attribute.getImplementations().contains(taskletImplName)) {
                    //  в старых схемах не было прикопана часть данных
                    if (attribute.getName() == null) {
                        attribute = attribute.withName(entry.getKey().substring(SCHEMA_ATTRIBUTE_PREFIX.length()));
                    }
                    if (attribute.getSbTask() == null) {
                        attribute = attribute.withSbTask("TASKLET_" + attribute.getName().toUpperCase());
                    }
                    return attribute;
                }
            } catch (JsonProcessingException e) {
                log.warn("cannot parse an attribute {}. Error: {}", entry.getKey(), e);
            }
        }

        throw new TaskletMetadataValidationException(String.format(
                "cannot find tasklet schema for implementation %s." +
                        " Checked attributes: %s. Checked implementations: %s. Sandbox resource: %s" +
                        " https://docs.yandex-team.ru/ci/job-tasklet#cannot-find-tasklet-schema",
                taskletImplName, attributes.keySet(), checkedImplementations, sandboxResourceId
        ));
    }

    @Override
    public List<JobResource> extractResources(TaskletMetadata metadata, SchemaOptions options, JsonObject output) {
        return schemaService.extractOutput(metadata, options, output);
    }

    @Override
    public TaskletSchema extractSchema(TaskletMetadata metadata, SchemaOptions schemaOptions) {
        return schemaService.extractSchema(metadata, schemaOptions);
    }

    @SuppressWarnings("ReferenceEquality")
    @Value
    @Builder
    @AllArgsConstructor
    public static class SchemaAttribute {
        @With
        String name;
        String input;
        String output;
        String meta;
        @With
        String sbTask;
        Features features;
        List<String> implementations;
    }
}
