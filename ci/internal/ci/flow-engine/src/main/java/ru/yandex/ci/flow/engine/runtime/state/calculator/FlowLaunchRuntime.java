package ru.yandex.ci.flow.engine.runtime.state.calculator;

import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import com.google.common.collect.HashMultimap;
import com.google.common.collect.Multimap;

import ru.yandex.ci.flow.engine.definition.common.UpstreamLink;
import ru.yandex.ci.flow.engine.definition.context.impl.UpstreamResourcesCollector;
import ru.yandex.ci.flow.engine.runtime.state.StageGroupState;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.JobState;
import ru.yandex.ci.flow.engine.source_code.SourceCodeService;

/**
 * Рантайм-отображение запуска флоу.
 * Является контейнером для {@link JobRuntime}, выстраивает их в иерархию.
 */
class FlowLaunchRuntime {
    private FlowLaunchEntity flowLaunch;
    @Nullable
    private final StageGroupState stageGroupState;
    private final SourceCodeService sourceCodeService;
    private final ResourceProvider resourceProvider;
    private final UpstreamResourcesCollector upstreamResourcesCollector;
    private final Map<String, JobRuntime> idMap = new HashMap<>();
    private final Multimap<JobRuntime, JobRuntime> downstreamMap = HashMultimap.create();

    FlowLaunchRuntime(
            FlowLaunchEntity flowLaunch,
            @Nullable StageGroupState stageGroupState,
            SourceCodeService sourceCodeService,
            ResourceProvider resourceProvider,
            UpstreamResourcesCollector upstreamResourcesCollector
    ) {
        this.flowLaunch = flowLaunch;
        this.stageGroupState = stageGroupState;
        this.sourceCodeService = sourceCodeService;
        this.resourceProvider = resourceProvider;
        this.upstreamResourcesCollector = upstreamResourcesCollector;

        for (JobState jobState : flowLaunch.getJobs().values()) {
            createJobRuntime(jobState);
        }

        for (JobRuntime jobRuntime : idMap.values()) {
            for (UpstreamLink<JobRuntime> upstreamJobRuntime : jobRuntime.getUpstreams()) {
                downstreamMap.put(upstreamJobRuntime.getEntity(), jobRuntime);
            }
        }
    }

    public void setFlowLaunch(FlowLaunchEntity flowLaunch) {
        this.flowLaunch = flowLaunch;
    }

    public FlowLaunchEntity getFlowLaunch() {
        return flowLaunch;
    }

    public JobRuntime getJob(String jobId) {
        var runtime =  idMap.get(jobId);
        if (runtime == null) {
            throw new IllegalArgumentException("Unable to find job with id " + jobId);
        }
        return runtime;
    }

    List<JobRuntime> getJobsWithoutDownstreams() {
        Collection<JobRuntime> allJobs = idMap.values();
        Set<JobRuntime> jobsWithDownstreams = downstreamMap.keySet();

        return allJobs.stream()
                .filter(j -> !jobsWithDownstreams.contains(j))
                .collect(Collectors.toList());
    }

    private JobRuntime createJobRuntime(JobState jobState) {
        String jobId = jobState.getJobId();

        if (idMap.containsKey(jobId)) {
            return idMap.get(jobId);
        }

        JobRuntime jobRuntime = new JobRuntime(
                jobState.getUpstreams().stream()
                        .map(kv -> new UpstreamLink<>(
                                createJobRuntime(this.flowLaunch.getJobState(kv.getEntity())), kv.getType(),
                                kv.getStyle()
                        )).collect(Collectors.toSet()),
                jobState,
                flowLaunch,
                sourceCodeService.lookupJobExecutor(jobState.getExecutorContext()).orElse(null),
                resourceProvider,
                upstreamResourcesCollector,
                stageGroupState
        );

        idMap.put(jobId, jobRuntime);
        return jobRuntime;
    }
}
