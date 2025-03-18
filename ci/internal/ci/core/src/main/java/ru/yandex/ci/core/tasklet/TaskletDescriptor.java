package ru.yandex.ci.core.tasklet;

import java.util.Map;

import com.google.protobuf.Descriptors.Descriptor;

import ru.yandex.ci.core.job.JobResourceType;

public interface TaskletDescriptor {

    Object getId();

    Map<String, Descriptor> getMessageDescriptors();

    JobResourceType getInputType();

    JobResourceType getOutputType();
}
