package ru.yandex.ci.flow.engine;

import java.util.ArrayDeque;
import java.util.HashSet;
import java.util.Map;
import java.util.Queue;
import java.util.Set;

import ru.yandex.ci.core.job.JobResourceType;
import ru.yandex.ci.flow.engine.definition.common.UpstreamLink;
import ru.yandex.ci.flow.engine.definition.job.AbstractJob;
import ru.yandex.ci.flow.engine.source_code.model.ConsumedResource;

public abstract class AbstractResourceProvider<JOB extends AbstractJob<JOB>, RESOURCE_CONTAINER, BUILDER> {
    protected AbstractResourceProvider() {
    }

    /**
     * Проходится BFS по апстримам и собирает нужные ресурсы c:
     * <ul>
     * <li>со всех апстримов (непосредственных и транизитивных) для данной джобы</li>
     * <li>ресурсов, заданных при старте</li>
     * </ul>
     */
    protected void addUpstreamResourcesToResult(
            Set<UpstreamLink<JOB>> startLinks,
            Map<JobResourceType, ConsumedResource> resourceDependencyMap,
            BUILDER resultBuilder
    ) {
        Queue<UpstreamLink<JOB>> bfsQueue = new ArrayDeque<>();
        bfsQueue.addAll(startLinks);

        Set<String> visitedJobIds = new HashSet<>();

        while (!bfsQueue.isEmpty()) {
            UpstreamLink<JOB> upstreamLink = bfsQueue.poll();
            JOB upstreamJob = upstreamLink.getEntity();

            if (visitedJobIds.contains(upstreamJob.getId())) {
                continue;
            }

            visitedJobIds.add(upstreamJob.getId());

            bfsQueue.addAll(
                    processUpstreamResource(
                            upstreamLink,
                            upstreamJob,
                            resourceDependencyMap,
                            resultBuilder
                    )
            );
        }
    }

    private Set<UpstreamLink<JOB>> processUpstreamResource(
            UpstreamLink<JOB> upstreamLink,
            JOB upstreamJob,
            Map<JobResourceType, ConsumedResource> resourceDependencyMap,
            BUILDER resultBuilder) {
        RESOURCE_CONTAINER producedResources = getProducedResources(upstreamJob);

        switch (upstreamLink.getType()) {
            case NO_RESOURCES:
                break;
            case DIRECT_RESOURCES:
                addResourcesToResult(resourceDependencyMap, producedResources, upstreamJob, resultBuilder);
                break;
            case ALL_RESOURCES:
                addResourcesToResult(resourceDependencyMap, producedResources, upstreamJob, resultBuilder);
                return upstreamJob.getUpstreams();
            default:
                throw new RuntimeException("Upstream of unknown type: " + upstreamLink.getType());
        }

        return Set.of();
    }

    protected abstract RESOURCE_CONTAINER getProducedResources(JOB upstreamJob);

    protected abstract void addResourcesToResult(Map<JobResourceType, ConsumedResource> resourceDependencyMap,
                                                 RESOURCE_CONTAINER producedResources,
                                                 JOB resourceParentJob,
                                                 BUILDER resultBuilder);
}
