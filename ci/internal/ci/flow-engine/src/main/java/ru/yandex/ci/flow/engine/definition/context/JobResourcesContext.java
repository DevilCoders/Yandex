package ru.yandex.ci.flow.engine.definition.context;

import java.util.Collection;
import java.util.List;
import java.util.function.Function;
import java.util.stream.Stream;

import com.google.protobuf.GeneratedMessageV3;

import ru.yandex.ci.core.job.JobResource;
import ru.yandex.ci.core.job.JobResources;
import ru.yandex.ci.core.resources.Resource;
import ru.yandex.ci.flow.engine.runtime.di.model.StoredResourceContainer;

public interface JobResourcesContext {
    /**
     * Gets produces resources.
     *
     * @return produced resources.
     */
    Collection<Resource> getProducedResources();

    /**
     * Gets produces resources.
     *
     * @return Produced resource container.
     */
    StoredResourceContainer getStoredProducedResources();

    /**
     * Produces resource.
     *
     * @param resource a resource to produce.
     */
    void produce(Resource resource);

    /**
     * Produces protobuf resource.
     *
     * @param message a protobuf message for producing as a resource.
     */
    void produce(GeneratedMessageV3 message);

    /**
     * @return job resources context
     */
    JobResources getJobResources();

    /**
     * Resolve job resources (replace JMES expressions with actual values)
     *
     * @param resolver resolver for each resource
     */
    void resolveJobResources(Function<JobResource, Stream<JobResource>> resolver);

    /**
     * Get consumed resource of provided type
     *
     * @param protoType resouce type
     * @param <T>       proto message
     * @return resource (or raise an exception if no such resource available)
     */
    <T extends GeneratedMessageV3> T consume(Class<T> protoType);

    /**
     * Get all consumed resources of provided type
     *
     * @param protoType resource type
     * @param <T>       proto message
     * @return list of resources (0 or more)
     */
    <T extends GeneratedMessageV3> List<T> consumeList(Class<T> protoType);
}
