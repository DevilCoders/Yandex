package ru.yandex.ci.core.job;

import javax.annotation.Nonnull;

import com.google.protobuf.Descriptors;
import com.google.protobuf.GeneratedMessageV3;
import com.google.protobuf.Message;
import lombok.Value;

import ru.yandex.ci.core.proto.ProtobufReflectionUtils;
import ru.yandex.ci.core.sandbox.SandboxExecutorContext;
import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value
public class JobResourceType {
    /**
     * Для входных параметров sandbox-задач не используется protobuf сериализация,
     * поэтому тип генерируется по имени sandbox-задачи.
     * Так как нет protobuf-сериализации, то могут предоставляться только как
     * PrepareLaunchParameters.Builder#withManualResources.
     **/
    private static final String SANDBOX_PARAMETERS_TYPE_PREFIX = "sb-task-input:";

    // Аналогично предыдущему, но это уже параметры для передачи в контекст
    private static final String SANDBOX_CONTEXT_PARAMETERS_TYPE_PREFIX = "sb-task-context:";

    @Nonnull
    String messageName;

    public static JobResourceType of(String messageName) {
        return new JobResourceType(messageName);
    }

    public static JobResourceType ofSandboxTask(SandboxExecutorContext sandboxTask) {
        return new JobResourceType(SANDBOX_PARAMETERS_TYPE_PREFIX + sandboxTask.getTaskType());
    }

    public static JobResourceType ofSandboxTaskContext(SandboxExecutorContext sandboxTask) {
        return new JobResourceType(SANDBOX_CONTEXT_PARAMETERS_TYPE_PREFIX + sandboxTask.getTaskType());
    }

    public static JobResourceType ofDescriptor(Descriptors.Descriptor descriptor) {
        return new JobResourceType(descriptor.getFullName());
    }

    public static JobResourceType ofMessage(Message message) {
        return ofDescriptor(message.getDescriptorForType());
    }

    // Relatively slow method
    public static JobResourceType ofField(Descriptors.FieldDescriptor field) {
        return ofDescriptor(field.getMessageType());
    }

    public static JobResourceType ofMessageClass(Class<? extends GeneratedMessageV3> clazz) {
        return ofDescriptor(ProtobufReflectionUtils.getMessageDescriptor(clazz));
    }

    public static boolean isSandboxTaskContext(JobResourceType type) {
        return type.messageName.startsWith(SANDBOX_CONTEXT_PARAMETERS_TYPE_PREFIX);
    }

}
