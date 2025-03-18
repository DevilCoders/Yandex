package ru.yandex.ci.core.taskletv2;

import java.util.UUID;
import java.util.function.Function;

import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Message;
import lombok.RequiredArgsConstructor;

import ru.yandex.ci.client.taskletv2.TaskletV2TestServer;
import ru.yandex.ci.core.proto.ProtobufReflectionUtils;
import ru.yandex.ci.core.tasklet.TaskletMetadataHelper;
import ru.yandex.tasklet.Result;
import ru.yandex.tasklet.TaskletAction;

@RequiredArgsConstructor
public class TaskletV2MetadataHelper {

    private final TaskletV2TestServer taskletV2TestServer;

    public TaskletV2Metadata.Id registerSchema(
            TaskletV2Metadata.Description description,
            Descriptor inputMessageDescriptor,
            Descriptor outputMessageDescriptor
    ) {
        var buildId = registerSchema(inputMessageDescriptor, outputMessageDescriptor);
        taskletV2TestServer.registerLabel(
                description.getNamespace(),
                description.getTasklet(),
                description.getLabel(),
                buildId.getId()
        );
        return buildId;
    }

    public void unregisterLabel(TaskletV2Metadata.Description description) {
        taskletV2TestServer.unregisterLabel(
                description.getNamespace(),
                description.getTasklet(),
                description.getLabel()
        );
    }

    public TaskletV2Metadata.Id registerSchema(
            Descriptor inputMessageDescriptor,
            Descriptor outputMessageDescriptor
    ) {
        var buildId = UUID.randomUUID().toString();
        var schemaHash = UUID.randomUUID().toString();

        taskletV2TestServer.registerBuild(
                buildId,
                schemaHash,
                inputMessageDescriptor.getFullName(),
                outputMessageDescriptor.getFullName()
        );
        taskletV2TestServer.registerSchema(
                schemaHash,
                TaskletMetadataHelper.descriptorSetFrom(inputMessageDescriptor, outputMessageDescriptor)
        );
        return TaskletV2Metadata.Id.of(buildId);
    }

    public <Input extends Message, Output extends Message> void registerExecutor(
            TaskletV2Metadata.Description description,
            Class<Input> inputType,
            Class<Output> outputType,
            Function<Input, Output> action
    ) {
        registerExecutor(description, inputType, outputType,
                (input, services) -> Result.of(action.apply(input)));
    }

    public <Input extends Message, Output extends Message> void registerExecutor(
            TaskletV2Metadata.Description description,
            Class<Input> inputType,
            Class<Output> outputType,
            TaskletAction<Input, Output> action
    ) {
        var id = registerSchema(
                description,
                ProtobufReflectionUtils.getMessageDescriptor(inputType),
                ProtobufReflectionUtils.getMessageDescriptor(outputType)
        );
        taskletV2TestServer.registerExecutor(id.getId(), inputType, outputType, action);
    }
}
