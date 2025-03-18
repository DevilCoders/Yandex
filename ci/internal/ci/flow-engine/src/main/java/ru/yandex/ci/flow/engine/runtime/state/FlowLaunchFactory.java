package ru.yandex.ci.flow.engine.runtime.state;

import java.time.Instant;
import java.util.Collection;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.stream.Collectors;

import ru.yandex.ci.flow.engine.definition.common.UpstreamLink;
import ru.yandex.ci.flow.engine.definition.job.Job;
import ru.yandex.ci.flow.engine.runtime.di.ResourceEntity;
import ru.yandex.ci.flow.engine.runtime.di.ResourceService;
import ru.yandex.ci.flow.engine.runtime.di.model.ResourceRefContainer;
import ru.yandex.ci.flow.engine.runtime.di.model.StoredResourceContainer;
import ru.yandex.ci.flow.engine.runtime.state.model.DelegatedOutputResources;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.JobState;
import ru.yandex.ci.flow.engine.source_code.SourceCodeService;

public class FlowLaunchFactory {
    private final ResourceService resourceService;
    private final SourceCodeService sourceCodeService;

    public FlowLaunchFactory(
            ResourceService resourceService, SourceCodeService sourceCodeService
    ) {
        this.resourceService = resourceService;
        this.sourceCodeService = sourceCodeService;
    }

    public FlowLaunchEntity create(FlowLaunchParameters parameters) {
        var bootstrap = parameters.getLaunchParameters();

        var flow = bootstrap.getFlow();

        var jobResources = new HashMap<>(parameters.getLaunchParameters().getJobResources());
        var jobStateMap = collectJobStates(flow.getJobs(), parameters, jobResources);

        // jobResources cannot be used in cleanupJobs
        var cleanupJobStateMap = collectJobStates(flow.getCleanupJobs(), parameters, new HashMap<>());

        var manualResources = jobResources.isEmpty()
                ? ResourceRefContainer.empty()
                : toRefContainer(jobResources);

        return FlowLaunchEntity.builder()
                .id(parameters.getFlowLaunchId())
                .launchId(bootstrap.getLaunchId())
                .launchInfo(bootstrap.getLaunchInfo())
                .vcsInfo(bootstrap.getVcsInfo())
                .flowInfo(bootstrap.getFlowInfo())
                .manualResources(manualResources)
                .jobs(jobStateMap)
                .cleanupJobs(cleanupJobStateMap)
                .createdDate(Instant.now())
                .rawStages(flow.getStages())
                .projectId(bootstrap.getProjectId())
                .triggeredBy(bootstrap.getTriggeredBy())
                .title(bootstrap.getTitle())
                .build();
    }


    private ResourceRefContainer toRefContainer(Map<String, DelegatedOutputResources> jobResources) {
        var allResources = jobResources.values().stream()
                .map(DelegatedOutputResources::getResourceRef)
                .map(ResourceRefContainer::getResources)
                .flatMap(Collection::stream)
                .toList();
        return ResourceRefContainer.of(allResources);
    }

    private Map<String, JobState> collectJobStates(
            List<Job> jobs,
            FlowLaunchParameters parameters,
            HashMap<String, DelegatedOutputResources> jobResources
    ) {
        Map<String, JobState> jobStateMap = new LinkedHashMap<>(jobs.size());
        for (Job job : jobs) {
            String jobId = job.getId();
            Set<UpstreamLink<String>> upstreamJobIdsToUpstreamTypes = job.getUpstreams()
                    .stream()
                    .map(u -> new UpstreamLink<>(u.getEntity().getId(), u.getType(), u.getStyle()))
                    .collect(Collectors.toSet());

            StoredResourceContainer staticResources = resourceService.saveResources(
                    job.getStaticResources(), parameters, ResourceEntity.ResourceClass.STATIC
            );

            try {
                this.sourceCodeService.validateJobExecutor(job.getExecutorContext());
            } catch (RuntimeException e) {
                throw new RuntimeException(String.format("Job %s is invalid: %s", job.getId(), e.getMessage()), e);
            }

            var delegatedResources = jobResources.remove(jobId);
            jobStateMap.put(jobId,
                    new JobState(job, upstreamJobIdsToUpstreamTypes, staticResources.toRefs(), delegatedResources)
            );
        }

        return jobStateMap;
    }
}
