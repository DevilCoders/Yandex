package ru.yandex.ci.flow.engine.source_code.impl;

import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import ru.yandex.ci.core.job.JobResourceType;
import ru.yandex.ci.core.proto.ProtobufReflectionUtils;
import ru.yandex.ci.core.resources.Resource;
import ru.yandex.ci.flow.engine.definition.resources.Consume;
import ru.yandex.ci.flow.engine.definition.resources.ProducedResourcesValidationException;
import ru.yandex.ci.flow.engine.source_code.model.ConsumedResource;
import ru.yandex.ci.flow.engine.source_code.model.ResourceObject;

public class ConsumedResourcesLoader {
    private ConsumedResourcesLoader() {
        //
    }

    public static List<ConsumedResource> load(Class<?> executorClass) {
        return ProducedResourcesLoader.loadImpl(executorClass, ConsumedResourcesLoader::getConsumedResources);
    }

    private static Collection<ConsumedResource> getConsumedResources(Class<?> executorClass) {
        var annotations = executorClass.getDeclaredAnnotationsByType(Consume.class);
        var result = new HashMap<JobResourceType, ConsumedResource>();
        for (var annotation : annotations) {
            fill(annotation, result);
        }
        return result.values();
    }

    private static void fill(Consume annotation, Map<JobResourceType, ConsumedResource> map) {
        var resourceType = JobResourceType.ofMessageClass(annotation.proto());
        var descriptor = ProtobufReflectionUtils.getMessageDescriptor(annotation.proto());
        var current = new ConsumedResource(
                annotation.name(),
                descriptor,
                new ResourceObject(
                        Resource.uuidFromMessageName(resourceType.getMessageName()),
                        Resource.class,
                        false,
                        resourceType.getMessageName(),
                        "Protobuf сообщение " + resourceType.getMessageName(),
                        resourceType
                ),
                annotation.list()
        );
        var previous = map.put(resourceType, current);
        if (previous != null) {
            throw new ProducedResourcesValidationException(
                    "Multiple definitions for protobuf message " + resourceType.getMessageName()
            );
        }
    }
}
