package ru.yandex.ci.flow.engine.definition.context.impl;

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.atomic.AtomicReference;
import java.util.function.Function;
import java.util.function.Supplier;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import javax.annotation.Nonnull;

import com.google.common.base.Preconditions;
import com.google.common.base.Suppliers;
import com.google.common.collect.HashMultimap;
import com.google.common.collect.Multimap;
import com.google.gson.JsonObject;
import com.google.protobuf.GeneratedMessageV3;
import com.google.protobuf.InvalidProtocolBufferException;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.core.job.JobResource;
import ru.yandex.ci.core.job.JobResourceType;
import ru.yandex.ci.core.job.JobResources;
import ru.yandex.ci.core.proto.ProtobufReflectionUtils;
import ru.yandex.ci.core.resources.Resource;
import ru.yandex.ci.flow.engine.definition.context.JobResourcesContext;
import ru.yandex.ci.flow.engine.definition.resources.ProducedResourcesValidationException;
import ru.yandex.ci.flow.engine.runtime.di.ResourceService;
import ru.yandex.ci.flow.engine.runtime.di.model.AbstractResourceContainer;
import ru.yandex.ci.flow.engine.runtime.di.model.StoredResource;
import ru.yandex.ci.flow.engine.runtime.di.model.StoredResourceContainer;
import ru.yandex.ci.flow.engine.source_code.model.ProducedResource;
import ru.yandex.ci.util.gson.CiGson;

@Slf4j
@RequiredArgsConstructor
public class JobResourcesContextImpl implements JobResourcesContext {
    private final Multimap<JobResourceType, Resource> producedResources = HashMultimap.create();

    private final AtomicReference<JobResources> consumedAndResolvedResources = new AtomicReference<>();
    private final Supplier<JobResources> consumedResources = Suppliers.memoize(this::loadJobResources);

    @Nonnull
    private final JobContextImpl sourceContext;
    @Nonnull
    private final UpstreamResourcesCollector upstreamResourcesCollector;
    @Nonnull
    private final ResourceService resourceService;

    @Override
    public JobResources getJobResources() {
        var resolved = consumedAndResolvedResources.get();
        return resolved != null
                ? resolved
                : consumedResources.get();
    }

    @Override
    public void resolveJobResources(Function<JobResource, Stream<JobResource>> resolver) {
        Preconditions.checkState(consumedAndResolvedResources.get() == null,
                "Resources are resolved already");
        var resources = consumedResources.get();
        var resolvedList = resources.getResources().stream()
                .flatMap(resolver)
                .toList();
        consumedAndResolvedResources.set(resources.withResources(resolvedList));
    }

    @Override
    public Collection<Resource> getProducedResources() {
        return this.producedResources.values();
    }

    @Override
    public StoredResourceContainer getStoredProducedResources() {
        var expectedResources = sourceContext.getExecutorObject().getProducedResources().values();
        if (expectedResources.isEmpty()) {
            return StoredResourceContainer.empty();
        }

        checkAllExpectedResourcesProduced(expectedResources);
        checkNoUnexpectedResourcesProduced(expectedResources);

        HashMultimap<JobResourceType, StoredResource> storedResourceMap = HashMultimap.create();

        for (Map.Entry<JobResourceType, Resource> entry : producedResources.entries()) {
            storedResourceMap.put(
                    entry.getKey(),
                    sourceContext.getFlowLaunch().fromResource(entry.getValue())
            );
        }

        return new StoredResourceContainer(storedResourceMap);
    }

    @Override
    public void produce(Resource resource) {
        Preconditions.checkArgument(resource != null, "Resource can't be null");

        log.info("Resource produced {} ({})", resource.getClass().getName(), resource.getSourceCodeId());

        producedResources.put(resource.getResourceType(), resource);
    }

    @Override
    public void produce(GeneratedMessageV3 message) {
        Preconditions.checkArgument(message != null, "Protobuf message can't be null");

        var resource = Resource.of(message);

        log.info(
                "Protobuf resource produced {} ({})",
                resource.getResourceType().getMessageName(), resource.getSourceCodeId()
        );

        producedResources.put(resource.getResourceType(), resource);
    }

    @Override
    public <T extends GeneratedMessageV3> T consume(Class<T> protoType) {
        var result = consumeList(protoType);
        if (result.isEmpty()) {
            throw new RuntimeException("Unable to find resource with type " + protoType);
        } else if (result.size() > 1) {
            throw new RuntimeException("Found " + result.size() + " resources with type " + protoType);
        } else {
            return result.get(0);
        }
    }

    @SuppressWarnings("unchecked")
    @Override
    public <T extends GeneratedMessageV3> List<T> consumeList(Class<T> protoType) {
        var result = new ArrayList<T>();
        var type = JobResourceType.ofMessageClass(protoType);
        for (var resource : getJobResources().getResources()) {
            if (type.equals(resource.getResourceType())) {
                var builder = ProtobufReflectionUtils.getMessageBuilder(protoType);
                try {
                    result.add((T) ProtobufReflectionUtils.merge(Resource.of(resource), builder).build());
                } catch (InvalidProtocolBufferException e) {
                    throw new RuntimeException("Unable to merge resources of type " + type, e);
                }
            }
        }
        return result;
    }

    @Nonnull
    public ResourceService getResourceService() {
        return resourceService;
    }

    private void checkAllExpectedResourcesProduced(Collection<ProducedResource> expectedResources) {
        for (ProducedResource expectedResource : expectedResources) {
            JobResourceType resourceType = expectedResource.getResource().getResourceType();
            boolean isList = expectedResource.isList();
            var resources = this.producedResources.get(resourceType);

            if (!isList && resources.size() != 1) {
                throw new ProducedResourcesValidationException(
                        "Job " + sourceContext.getJobId() +
                                " must produce exactly one instance of resource " + resourceType +
                                ", flow launch ID = " + sourceContext.getFlowLaunch().getFlowLaunchId() +
                                ", produced resources = " + CiGson.instance().toJson(resources)
                );
            }
        }
    }

    private void checkNoUnexpectedResourcesProduced(Collection<ProducedResource> expectedResources) {
        Set<JobResourceType> expectedResourceClasses = expectedResources.stream()
                .map(r -> r.getResource().getResourceType())
                .collect(Collectors.toSet());
        for (JobResourceType producedResourceClass : producedResources.keys()) {
            if (!expectedResourceClasses.contains(producedResourceClass)) {
                throw new ProducedResourcesValidationException(
                        "Job " + sourceContext.getJobId() +
                                " must not produce resource " + producedResourceClass +
                                ", flow launch ID = " + sourceContext.getFlowLaunch().getFlowLaunchId() +
                                ", allowed resources = " + expectedResourceClasses.stream()
                                .map(JobResourceType::toString)
                                .collect(Collectors.joining(", "))
                );
            }
        }
    }

    private AbstractResourceContainer loadConsumedResources() {
        return this.resourceService.loadResources(sourceContext.getJobState().getLastLaunch().getConsumedResources());
    }

    private Map<String, JsonObject> loadUpstreamResources() {
        return upstreamResourcesCollector.collectUpstreamResources(
                sourceContext.getFlowLaunch(), sourceContext.getJobState());
    }

    private JobResources loadJobResources() {
        return JobResources.of(toJobResources(loadConsumedResources()), this::loadUpstreamResources);
    }

    private static List<JobResource> toJobResources(AbstractResourceContainer resourceContainer) {
        return resourceContainer.getAll().stream()
                .map(JobResource::of)
                .collect(Collectors.toList());
    }
}
