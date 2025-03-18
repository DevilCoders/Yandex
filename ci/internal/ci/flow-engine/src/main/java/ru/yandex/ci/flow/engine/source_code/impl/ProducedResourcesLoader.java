package ru.yandex.ci.flow.engine.source_code.impl;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.function.Function;

import com.google.protobuf.GeneratedMessageV3;

import ru.yandex.ci.core.job.JobResourceType;
import ru.yandex.ci.core.resources.Resource;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;
import ru.yandex.ci.flow.engine.definition.resources.ProducedResourcesValidationException;
import ru.yandex.ci.flow.engine.definition.resources.Produces;
import ru.yandex.ci.flow.engine.source_code.model.AbstractResource;
import ru.yandex.ci.flow.engine.source_code.model.ProducedResource;
import ru.yandex.ci.flow.engine.source_code.model.ResourceObject;

public class ProducedResourcesLoader {

    private ProducedResourcesLoader() {
        //
    }

    public static List<ProducedResource> load(Class<?> executorClass) {
        return loadImpl(executorClass, ProducedResourcesLoader::getProducedResources);
    }

    static <T extends AbstractResource> List<T> loadImpl(
            Class<?> executorClass,
            Function<Class<?>, Collection<T>> action
    ) {
        var result = new ArrayList<T>();
        // Separate unique check because same resource id can be produced as single in parent and as multiple in child,
        // and in this case they won't be equals by value.
        var resourceIds = new HashSet<UUID>();
        Class<?> clazz = executorClass;
        while (clazz != Object.class &&
                JobExecutor.class.isAssignableFrom(clazz)) {
            var values = action.apply(clazz);
            values.stream()
                    .filter(resource -> !resourceIds.contains(resource.getId()))
                    .forEach(resource -> {
                        result.add(resource);
                        resourceIds.add(resource.getId());
                    });
            clazz = clazz.getSuperclass();
        }
        return result;
    }

    private static Collection<ProducedResource> getProducedResources(Class<?> executorClass) {
        var producesAnnotations = executorClass.getDeclaredAnnotationsByType(Produces.class);
        var expectedProtoResources = new HashMap<JobResourceType, ProducedResource>();

        for (var producesAnnotation : producesAnnotations) {
            // Resources from generated protobuf messages
            fill(producesAnnotation.single(), false, expectedProtoResources);
            fill(producesAnnotation.multiple(), true, expectedProtoResources);
        }

        return expectedProtoResources.values();
    }

    private static void fill(
            Class<? extends GeneratedMessageV3>[] messages,
            boolean list,
            Map<JobResourceType, ProducedResource> map
    ) {
        for (var resourceClass : messages) {
            var resourceType = JobResourceType.ofMessageClass(resourceClass);
            var current = new ProducedResource(
                    new ResourceObject(
                            Resource.uuidFromMessageName(resourceType.getMessageName()),
                            Resource.class,
                            false,
                            resourceType.getMessageName(),
                            "Protobuf сообщение " + resourceType.getMessageName(),
                            resourceType
                    ),
                    list
            );
            var previous = map.put(resourceType, current);
            if (previous != null) {
                throw new ProducedResourcesValidationException(
                        "Multiple definitions for protobuf message " + resourceType.getMessageName()
                );
            }
        }
    }
}
