package ru.yandex.ci.core.tasklet;

import java.util.LinkedHashSet;
import java.util.Set;

import com.google.protobuf.DescriptorProtos.FileDescriptorSet;
import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.FileDescriptor;

import ru.yandex.ci.core.job.JobResourceType;

public class TaskletMetadataHelper {

    private TaskletMetadataHelper() {
        //
    }

    public static TaskletMetadata metadataFor(
            String implementation,
            long resourceId,
            Descriptor taskletDescriptor,
            Descriptor inputDescriptor,
            Descriptor outputDescriptor
    ) {
        FileDescriptorSet descriptors = descriptorSetFrom(taskletDescriptor);
        return new TaskletMetadata.Builder()
                .id(TaskletMetadata.Id.of(implementation, TaskletRuntime.SANDBOX, resourceId))
                .descriptors(descriptors.toByteArray())
                .features(Features.builder().consumesSecretId(true).build())
                .inputType(JobResourceType.ofDescriptor(inputDescriptor))
                .outputType(JobResourceType.ofDescriptor(outputDescriptor))
                .build();
    }

    private static void descriptorSetFrom(FileDescriptor current, Set<FileDescriptor> result) {
        if (!result.contains(current)) {
            for (FileDescriptor dependency : current.getDependencies()) {
                descriptorSetFrom(dependency, result);
            }
            result.add(current);
        }
    }

    public static FileDescriptorSet descriptorSetFrom(Descriptor... descriptors) {

        Set<FileDescriptor> files = new LinkedHashSet<>();
        for (var descriptor : descriptors) {
            descriptorSetFrom(descriptor.getFile(), files);
        }

        FileDescriptorSet.Builder result = FileDescriptorSet.newBuilder();
        files.stream()
                .map(FileDescriptor::toProto)
                .forEach(result::addFile);

        return result.build();
    }
}
