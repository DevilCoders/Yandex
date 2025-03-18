package ru.yandex.ci.tms.test;

import java.io.ByteArrayOutputStream;
import java.io.UncheckedIOException;
import java.time.Instant;
import java.util.ArrayList;
import java.util.Base64;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicLong;
import java.util.concurrent.atomic.AtomicReference;
import java.util.function.Consumer;
import java.util.function.Function;
import java.util.stream.Collectors;
import java.util.zip.Deflater;

import javax.annotation.Nonnull;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.common.base.Preconditions;
import com.google.gson.Gson;
import com.google.protobuf.DescriptorProtos;
import com.google.protobuf.Descriptors;
import com.google.protobuf.GeneratedMessage.GeneratedExtension;
import com.google.protobuf.InvalidProtocolBufferException;
import com.google.protobuf.Message;
import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.RequiredArgsConstructor;
import lombok.Singular;
import lombok.Value;
import one.util.streamex.StreamEx;
import tasklet.Tasklet;

import ru.yandex.ci.client.base.http.HttpException;
import ru.yandex.ci.client.sandbox.SandboxClient;
import ru.yandex.ci.client.sandbox.SandboxResponse;
import ru.yandex.ci.client.sandbox.TasksFilter;
import ru.yandex.ci.client.sandbox.api.BatchResult;
import ru.yandex.ci.client.sandbox.api.BatchStatus;
import ru.yandex.ci.client.sandbox.api.DelegationResultList;
import ru.yandex.ci.client.sandbox.api.ResourceInfo;
import ru.yandex.ci.client.sandbox.api.ResourceState;
import ru.yandex.ci.client.sandbox.api.Resources;
import ru.yandex.ci.client.sandbox.api.SandboxBatchAction;
import ru.yandex.ci.client.sandbox.api.SandboxCustomField;
import ru.yandex.ci.client.sandbox.api.SandboxTask;
import ru.yandex.ci.client.sandbox.api.SandboxTaskOutput;
import ru.yandex.ci.client.sandbox.api.SandboxTaskStatus;
import ru.yandex.ci.client.sandbox.api.SecretList;
import ru.yandex.ci.client.sandbox.api.TaskAuditRecord;
import ru.yandex.ci.client.sandbox.model.Semaphore;
import ru.yandex.ci.core.proto.ProtobufReflectionUtils;
import ru.yandex.ci.core.proto.ProtobufSerialization;
import ru.yandex.ci.core.tasklet.TaskletMetadataServiceImpl.SchemaAttribute;
import ru.yandex.ci.engine.flow.SandboxClientFactory;

public class SandboxClientTaskletExecutorStub implements SandboxClient, SandboxClientFactory {
    private static final long TASK_ID_START = 829400000;
    private static final String SANDBOX_TASKS_BINARY = "SANDBOX_TASKS_BINARY";

    private final Map<Long, TaskletResourceForTest> tasklets = new HashMap<>();
    private final Map<String, SandboxResourceForTest> sandboxes = new HashMap<>();

    private final Map<Long, Task> tasks = new ConcurrentHashMap<>();
    private final Map<Long, Resources> taskResources = new ConcurrentHashMap<>();

    private final AtomicLong ids = new AtomicLong(TASK_ID_START);

    private boolean failStart = false;

    public void reset() {
        tasklets.clear();
        tasks.clear();
        ids.set(TASK_ID_START);
        failStart = false;
    }

    public void setFailStart(boolean failStart) {
        this.failStart = failStart;
    }

    public void uploadTasklet(long resourceId, TaskletStub<?, ?> taskletStub) {
        uploadTasklet(resourceId, taskletStub, new TaskletExecutor<>(taskletStub));
    }

    public void uploadTasklet(long resourceId, TaskletStub<?, ?> taskletStub, TaskExecutor customExecutor) {
        tasklets.put(resourceId,
                new TaskletResourceForTest(
                        taskletResource(resourceId, taskletStub.taskletMessage(), taskletStub.implementationName()),
                        customExecutor
                )
        );
    }

    public void uploadSandboxTask(String sandboxType, SandboxStub sandboxStub) {
        sandboxes.put(sandboxType, new SandboxResourceForTest(new SandboxExecutor(sandboxStub)));
    }

    @Override
    public SandboxTaskOutput createTask(SandboxTask sandboxTask) {
        Task.Builder builder = Task.builder()
                .sandboxTask(sandboxTask)
                .status(SandboxTaskStatus.DRAFT);

        var task = sandboxes.get(Objects.requireNonNullElse(sandboxTask.getType(), sandboxTask.getTemplateAlias()));
        if (task != null) {
            builder.executor(task.getTaskExecutor());
        } else {
            Long tasksResource = sandboxTask.getRequirements().getTasksResource();
            if (tasksResource != null) {
                TaskletResourceForTest resourceInfo = tasklets.get(tasksResource);
                if (resourceInfo.getTaskExecutor() != null) {
                    builder.executor(resourceInfo.getTaskExecutor());
                }
            } else {
                throw new RuntimeException("Task not found, cannot start " + sandboxTask);
            }
        }
        long id = ids.incrementAndGet();
        builder.id(id);

        tasks.put(id, builder.build());
        return new SandboxTaskOutput(sandboxTask.getType(), id, SandboxTaskStatus.DRAFT);
    }

    @Override
    public BatchResult startTask(long taskId, String comment) {
        if (failStart) {
            throw new SandboxFakeException("Emulatted fail");
        }
        Task updated = tasks.compute(taskId, (id, task) -> {
                    if (task == null) {
                        return null;
                    }
                    return task.toBuilder()
                            .status(SandboxTaskStatus.EXECUTING)
                            .startTime(Instant.now())
                            .build();
                }
        );
        var result = BatchResult.builder();
        if (updated == null) {
            result.status(BatchStatus.ERROR);
            result.message("task " + taskId + " not found");
        } else {
            result.status(BatchStatus.SUCCESS);
        }
        return result.build();
    }

    @Override
    public List<BatchResult> startTasks(Set<Long> taskIds, String comment) {
        return taskIds.stream()
                .map(taskId -> startTask(taskId, comment))
                .collect(Collectors.toList());
    }

    @Override
    public BatchResult stopTask(long taskId, String comment) {
        return new BatchResult(taskId, null, BatchStatus.SUCCESS);
    }

    @Override
    public List<BatchResult> stopTasks(Set<Long> taskIds, String comment) {
        throw new UnsupportedOperationException();
    }

    @Override
    public List<BatchResult> executeBatchTaskAction(SandboxBatchAction action, Set<Long> taskIds, String comment) {
        throw new UnsupportedOperationException();
    }

    @Override
    public SandboxResponse<SandboxTaskOutput> getTask(long taskId) {
        Task task = tasks.get(taskId);
        task = task.executor.update(task);
        return defaultResponse(
                new SandboxTaskOutput(
                        task.getSandboxTask().getType(), taskId,
                        task.getStatus(),
                        StreamEx.of(task.getSandboxTask().getCustomFields())
                                .toMap(SandboxCustomField::getName, SandboxCustomField::getValue),
                        task.getOutputParameters(),
                        List.of()
                )
        );
    }

    @Override
    public SandboxResponse<SandboxTaskStatus> getTaskStatus(long taskId) {
        return defaultResponse(getTask(taskId).getData().getStatusEnum());
    }

    private <T> SandboxResponse<T> defaultResponse(T data) {
        return SandboxResponse.of(data, 21, 42);
    }

    @Override
    public SandboxResponse<List<TaskAuditRecord>> getTaskAudit(long taskId) {
        TaskAuditRecord auditRecord = new TaskAuditRecord(
                getTask(taskId).getData().getStatusEnum(),
                Instant.ofEpochSecond(1615505870),
                "Things changes sometimes",
                "robot-ci"
        );
        return defaultResponse(List.of(auditRecord));
    }

    @Override
    public Map<Long, SandboxTaskOutput> getTasks(Set<Long> taskIds, Set<String> fields) {
        return taskIds.stream()
                .map(this::getTask)
                .map(SandboxResponse::getData)
                .collect(Collectors.toMap(SandboxTaskOutput::getId, Function.identity()));
    }

    @Override
    public void getTasks(Set<Long> taskIds, Set<String> fields,
                         Consumer<Map<Long, SandboxTaskOutput>> responseConsumer) {

        Map<Long, SandboxTaskOutput> response = taskIds.stream()
                .map(this::getTask)
                .map(SandboxResponse::getData)
                .collect(Collectors.toMap(SandboxTaskOutput::getId, Function.identity()));
        responseConsumer.accept(response);
    }

    @Override
    public List<SandboxTaskOutput> getTasks(TasksFilter filter) {
        return List.of();
    }

    @Override
    public Resources getTaskResources(long taskId, String resourceType) {
        var resources = taskResources.get(taskId);
        if (resources == null) {
            throw new UnsupportedOperationException();
        } else {
            return resources;
        }
    }

    @Override
    public long getTotalFilteredTasks(TasksFilter filter) {
        return 0;
    }

    @Override
    public List<Long> getTasksIds(TasksFilter filter) {
        return List.of();
    }

    @Override
    public Semaphore getSemaphore(String id) {
        return null;
    }

    @Override
    public void setSemaphoreCapacity(String id, long capacity, String comment) {

    }

    @Override
    public ResourceInfo getResourceInfo(long resourceId) {
        TaskletResourceForTest resource = tasklets.get(resourceId);
        if (resource == null) {
            throw new HttpException("http://", 0, 404, "resource " + resourceId + " not found");
        }
        return resource.getResourceInfo();
    }

    @Override
    public DelegationResultList.DelegationResult delegateYavSecret(String secretUuid, String tvmUserTicket) {
        return null;
    }

    @Override
    public DelegationResultList delegateYavSecrets(SecretList secretList, String tvmUserTicket) {
        return null;
    }

    @Override
    public List<String> getCurrentUserGroups() {
        return List.of();
    }

    @Override
    public String getCurrentLogin() {
        return null;
    }

    @Override
    public int getMaxLimitForGetTasksRequest() {
        return SandboxClient.MAX_LIMIT_FOR_GET_TASKS_REQUEST;
    }

    @Nonnull
    @Override
    public SandboxClient create(@Nonnull String oauthToken) {
        return this;
    }

    private ResourceInfo taskletResource(long resourceId,
                                         Descriptors.Descriptor taskletMessage,
                                         String implementation) {
        Preconditions.checkArgument(taskletMessage.getOptions().hasExtension(Tasklet.taskletInterface),
                "message %s should be marked as %s",
                taskletMessage.getFullName(),
                Tasklet.taskletInterface.getDescriptor().getName()
        );
        var inputMessage = findField(taskletMessage, Tasklet.input);
        var outputMessage = findField(taskletMessage, Tasklet.output);
        var meta = serializeDescriptor(inputMessage, outputMessage);

        var attribute = SchemaAttribute.builder();
        String taskletMessageName = taskletMessage.getName();
        attribute.name("test_tasklet_name_" + taskletMessageName);
        attribute.input(inputMessage.getFullName());
        attribute.output(outputMessage.getFullName());
        attribute.meta(meta);
        attribute.sbTask("test_tasklet_sandbox_" + taskletMessageName);
        attribute.implementations(List.of(implementation));

        String attributeJson = new Gson().toJson(attribute.build());

        return ResourceInfo.builder()
                .id(resourceId)
                .type(SANDBOX_TASKS_BINARY)
                .description("test resource of tasklet")
                .state(ResourceState.READY)
                .attributes(Map.of("schema_" + taskletMessageName, attributeJson))
                .task(new ResourceInfo.Task(0, SandboxTaskStatus.SUCCESS, ""))
                .build();
    }

    private static Descriptors.Descriptor findField(Descriptors.Descriptor taskletMessage,
                                                    GeneratedExtension<DescriptorProtos.FieldOptions, ?> option) {
        for (Descriptors.FieldDescriptor field : taskletMessage.getFields()) {
            if (field.getOptions().hasExtension(option)) {
                return field.getMessageType();
            }
        }

        throw new RuntimeException(String.format("not found field with option %s in message %s",
                option.getDescriptor().getName(), taskletMessage.getFullName())
        );
    }

    private static String serializeDescriptor(Descriptors.Descriptor inputMessage,
                                              Descriptors.Descriptor outputMessage) {
        var fileSet = DescriptorProtos.FileDescriptorSet.newBuilder();
        var seen = new HashSet<String>();
        traverseDependencies(fileSet, inputMessage.getFile(), seen);
        traverseDependencies(fileSet, outputMessage.getFile(), seen);

        byte[] bytes = fileSet.build().toByteArray();
        var compresser = new Deflater(9);
        byte[] buffer = new byte[1024];
        compresser.setInput(bytes);
        compresser.finish();
        ByteArrayOutputStream zipped = new ByteArrayOutputStream();
        while (!compresser.finished()) {
            int read = compresser.deflate(buffer);
            zipped.write(buffer, 0, read);
        }
        return Base64.getEncoder().encodeToString(zipped.toByteArray());
    }

    private static void traverseDependencies(DescriptorProtos.FileDescriptorSet.Builder fileSet,
                                             Descriptors.FileDescriptor file, Set<String> seen) {
        if (!seen.add(file.getFullName())) {
            return;
        }

        for (Descriptors.FileDescriptor dependency : file.getDependencies()) {
            traverseDependencies(fileSet, dependency, seen);
        }

        fileSet.addFile(file.toProto());
    }

    @Value
    @Builder(toBuilder = true)
    public static class Task {
        long id;
        SandboxTask sandboxTask;
        SandboxTaskStatus status;
        Instant startTime;
        @Singular
        Map<String, Object> outputParameters;

        TaskExecutor executor;

        AtomicReference<Task> withResult = new AtomicReference<>();
    }

    public interface TaskExecutor {
        Task update(Task task);
    }

    public static class ManualStatusTaskExecutor implements TaskExecutor {
        private volatile SandboxTaskStatus taskStatus;

        public void setTaskStatus(SandboxTaskStatus taskStatus) {
            this.taskStatus = taskStatus;
        }

        @Override
        public Task update(Task task) {
            if (taskStatus != null) {
                return task.toBuilder()
                        .status(taskStatus)
                        .build();
            }
            return task;
        }
    }

    private static class TaskletExecutor<Input extends Message, Output extends Message> implements TaskExecutor {
        private final TaskletStub<Input, Output> tasklet;

        private TaskletExecutor(TaskletStub<Input, Output> tasklet) {
            this.tasklet = tasklet;
        }

        @Override
        public Task update(Task task) {
            if (task.getStatus() == SandboxTaskStatus.EXECUTING) {
                // Запрещаем повторное исполнение в рамках одного вызова Sandbox задачи
                // А повторное исполнение может случаться если что-то дернет проверку статуса
                var withResult = task.withResult.get();
                if (withResult != null) {
                    return withResult;
                }

                Output output = tasklet.execute(parseInput(task));

                Tasklet.JobResult result = Tasklet.JobResult.newBuilder()
                        .setSuccess(true)
                        .build();

                var lastResult = task.toBuilder()
                        .outputParameter("__tasklet_output__", ProtobufSerialization.serializeToGsonMap(output))
                        .outputParameter("tasklet_result", ProtobufSerialization.serializeToJsonString(result))
                        .status(SandboxTaskStatus.SUCCESS)
                        .build();
                task.withResult.set(lastResult);
                return lastResult;
            }
            return task;
        }

        @SuppressWarnings("unchecked")
        private Input parseInput(Task task) {
            try {
                Object taskletInput = StreamEx.of(task.getSandboxTask().getCustomFields())
                        .filterBy(SandboxCustomField::getName, "__tasklet_input__")
                        .map(SandboxCustomField::getValue)
                        .findFirst()
                        .orElseThrow();
                Map<String, Object> input = (Map<String, Object>) taskletInput;
                String json;
                try {
                    json = new ObjectMapper().writeValueAsString(input);
                } catch (JsonProcessingException e) {
                    throw new RuntimeException("cannot serialize tasklet input: " + input, e);
                }
                var inputBuilder = ProtobufSerialization.deserializeFromJsonString(json,
                        ProtobufReflectionUtils.getMessageBuilder(tasklet.inputClass()));
                //noinspection unchecked
                return (Input) inputBuilder.build();
            } catch (InvalidProtocolBufferException e) {
                throw new UncheckedIOException(e);
            }
        }
    }

    public interface TaskletStub<Input extends Message, Output extends Message> {
        String implementationName();

        Descriptors.Descriptor taskletMessage();

        Class<Input> inputClass();

        Output execute(Input input);
    }

    @Value
    private static class TaskletResourceForTest {
        ResourceInfo resourceInfo;
        TaskExecutor taskExecutor;
    }


    @AllArgsConstructor
    private class SandboxExecutor implements TaskExecutor {

        private final SandboxStub sandboxStub;

        @Override
        public Task update(Task task) {
            if (task.getStatus() == SandboxTaskStatus.EXECUTING) {
                // Запрещаем повторное исполнение в рамках одного вызова Sandbox задачи
                // А повторное исполнение может случаться если что-то дернет проверку статуса
                var withResult = task.withResult.get();
                if (withResult != null) {
                    return withResult;
                }
                var collector = new ResourceCollectorImpl(task);

                sandboxStub.execute(task.getSandboxTask(), collector);
                taskResources.put(task.id, new Resources(collector.resourceList));

                var lastResult = task.toBuilder()
                        .outputParameters(collector.outputMap)
                        .status(collector.taskStatus)
                        .build();
                task.withResult.set(lastResult);
                return lastResult;
            }
            return task;
        }
    }

    public interface ResourceCollector {
        void addResource(ResourceState state, String resourceType, Map<String, String> attributes);

        void setOutput(Map<String, Object> output);

        void setStatus(SandboxTaskStatus status);

        default void addResource(String resourceType, Map<String, String> attributes) {
            addResource(ResourceState.READY, resourceType, attributes);
        }

    }

    @RequiredArgsConstructor
    private class ResourceCollectorImpl implements ResourceCollector {
        private final Task task;
        private final List<ResourceInfo> resourceList = new ArrayList<>();
        private final Map<String, Object> outputMap = new HashMap<>();
        private SandboxTaskStatus taskStatus = SandboxTaskStatus.SUCCESS;

        @Override
        public void addResource(
                @Nonnull ResourceState state,
                @Nonnull String resourceType,
                @Nonnull Map<String, String> attributes
        ) {
            var id = ids.incrementAndGet();
            var resourceTask = new ResourceInfo.Task(task.getId(), task.getStatus(), "http://localhost");
            resourceList.add(
                    ResourceInfo.builder()
                            .id(id)
                            .type(resourceType)
                            .description("")
                            .state(state)
                            .attributes(attributes)
                            .task(resourceTask)
                            .build()
            );
        }

        @Override
        public void setOutput(@Nonnull Map<String, Object> output) {
            outputMap.clear();
            outputMap.putAll(output);
        }

        @Override
        public void setStatus(@Nonnull SandboxTaskStatus status) {
            taskStatus = status;
        }
    }

    public interface SandboxStub {
        void execute(SandboxTask task, ResourceCollector resourceCollector);
    }

    @Value
    private static class SandboxResourceForTest {
        TaskExecutor taskExecutor;
    }
}
