package ru.yandex.ci.flow.engine.runtime;

import java.nio.charset.StandardCharsets;
import java.util.Collection;
import java.util.List;
import java.util.Objects;
import java.util.UUID;
import java.util.function.BiFunction;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;

import lombok.RequiredArgsConstructor;

import ru.yandex.ci.core.job.JobResourceType;
import ru.yandex.ci.core.resources.Resource;
import ru.yandex.ci.core.sandbox.SandboxExecutorContext;
import ru.yandex.ci.core.tasklet.TaskletExecutorContext;
import ru.yandex.ci.core.tasklet.TaskletMetadataService;
import ru.yandex.ci.core.tasklet.TaskletSchema;
import ru.yandex.ci.core.taskletv2.TaskletV2ExecutorContext;
import ru.yandex.ci.core.taskletv2.TaskletV2MetadataService;
import ru.yandex.ci.flow.engine.runtime.state.model.ExecutorContext;
import ru.yandex.ci.flow.engine.source_code.model.ConsumedResource;
import ru.yandex.ci.flow.engine.source_code.model.JobExecutorObject;
import ru.yandex.ci.flow.engine.source_code.model.ProducedResource;
import ru.yandex.ci.flow.engine.source_code.model.ResourceObject;
import ru.yandex.ci.tasklet.SandboxResource;

@RequiredArgsConstructor
public class JobExecutorObjectProviderImpl implements JobExecutorObjectProvider {
    @Nonnull
    private final TaskletMetadataService taskletMetadataService;

    @Nonnull
    private final TaskletV2MetadataService taskletV2MetadataService;

    @Override
    public JobExecutorObject createExecutorObject(ExecutorContext executorContext) {
        // описания class-based executor-ов управляется SourceCodeService
        var executorType = ExecutorType.selectFor(executorContext);
        return switch (executorType) {
            case TASKLET ->
                    createTaskletExecutorObject(Objects.requireNonNull(executorContext.getTasklet()));
            case TASKLET_V2 ->
                    createTaskletV2ExecutorObject(Objects.requireNonNull(executorContext.getTaskletV2()));
            case SANDBOX_TASK ->
                    createSandboxTaskExecutorObject(Objects.requireNonNull(executorContext.getSandboxTask()));
            default -> throw new IllegalArgumentException("unexpected %s executor type".formatted(executorType));
        };
    }

    private JobExecutorObject createSandboxTaskExecutorObject(@Nonnull SandboxExecutorContext sandboxTask) {
        JobResourceType sandboxResourceType = JobResourceType.ofDescriptor(SandboxResource.getDescriptor());
        JobResourceType sandboxParamsType = JobResourceType.ofSandboxTask(sandboxTask);
        JobResourceType sandboxContextParamsType = JobResourceType.ofSandboxTaskContext(sandboxTask);

        List<ProducedResource> produced = List.of(new ProducedResource(objectFromType(sandboxResourceType), true));
        List<ConsumedResource> consumed = List.of(
                ConsumedResource.taskletResource(objectFromType(sandboxParamsType), false),
                ConsumedResource.taskletResource(objectFromType(sandboxContextParamsType), false)
        );

        return new JobExecutorObject(
                uuidFromString(sandboxTask.toString()),
                null,
                produced,
                consumed
        );
    }

    private JobExecutorObject createTaskletExecutorObject(@Nonnull TaskletExecutorContext context) {
        var taskletId = context.getTaskletKey();
        var taskletMetadata = taskletMetadataService.fetchMetadata(taskletId);
        var taskletSchema = taskletMetadataService.extractSchema(taskletMetadata, context.getSchemaOptions());

        return createExecutorObject(taskletId.toString(), taskletSchema);
    }

    private JobExecutorObject createTaskletV2ExecutorObject(@Nonnull TaskletV2ExecutorContext context) {
        var taskletId = context.getTaskletKey();
        var taskletMetadata = taskletV2MetadataService.fetchMetadata(taskletId);
        var taskletSchema = taskletV2MetadataService.extractSchema(taskletMetadata, context.getSchemaOptions());

        return createExecutorObject(taskletId.toString(), taskletSchema);
    }

    private JobExecutorObject createExecutorObject(String taskletId, TaskletSchema taskletSchema) {
        var produced = extractResources(
                taskletSchema.getOutput(),
                (type, isList) -> new ProducedResource(objectFromType(type), isList)
        );

        var consumed = extractResources(
                taskletSchema.getInput(),
                (type, isList) -> ConsumedResource.taskletResource(objectFromType(type), isList)
        );

        return new JobExecutorObject(
                uuidFromString(taskletId),
                null,
                produced,
                consumed
        );
    }

    private <T> List<T> extractResources(Collection<TaskletSchema.Field> fields,
                                         BiFunction<JobResourceType, Boolean, T> factory) {

        return fields.stream()
                .collect(Collectors.groupingBy(TaskletSchema.Field::getResourceType))
                .values().stream()
                .map(sameTypeFields -> {
                    // если тасклет принимает несколько ресурсов одного типа, однако в разные поля входа,
                    // информация о том, сколько конкретно ресурсов он ожидает тут теряется
                    // и джоба flow ожидает список, в котором может быть любое количество ресурсов
                    // в том числе меньше нужного
                    // https://st.yandex-team.ru/CI-946
                    TaskletSchema.Field first = sameTypeFields.get(0);
                    boolean isList = sameTypeFields.size() > 1 || first.isRepeated();
                    return factory.apply(first.getResourceType(), isList);
                })
                .collect(Collectors.toList());
    }

    private static ResourceObject objectFromType(JobResourceType resourceType) {
        return new ResourceObject(
                Resource.uuidFromMessageName(resourceType.getMessageName()),
                Resource.class,
                false,
                resourceType.getMessageName(),
                "Protobuf сообщение " + resourceType.getMessageName(),
                resourceType
        );
    }

    private static UUID uuidFromString(String string) {
        return UUID.nameUUIDFromBytes(string.getBytes(StandardCharsets.UTF_8));
    }
}
