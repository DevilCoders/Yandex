package ru.yandex.ci.flow.engine.definition.builder;

import java.util.ArrayList;
import java.util.Collection;
import java.util.IdentityHashMap;
import java.util.List;
import java.util.Map;
import java.util.NoSuchElementException;
import java.util.Objects;
import java.util.Set;
import java.util.UUID;
import java.util.function.BiFunction;
import java.util.function.Function;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import javax.annotation.Nonnull;

import com.google.common.annotations.VisibleForTesting;
import com.google.common.base.Preconditions;

import ru.yandex.ci.core.internal.InternalExecutorContext;
import ru.yandex.ci.core.sandbox.SandboxExecutorContext;
import ru.yandex.ci.core.tasklet.TaskletExecutorContext;
import ru.yandex.ci.core.taskletv2.TaskletV2ExecutorContext;
import ru.yandex.ci.flow.engine.definition.Flow;
import ru.yandex.ci.flow.engine.definition.common.UpstreamLink;
import ru.yandex.ci.flow.engine.definition.job.Job;
import ru.yandex.ci.flow.engine.definition.stage.Stage;
import ru.yandex.ci.flow.engine.definition.stage.StageGroup;
import ru.yandex.ci.flow.engine.runtime.state.model.JobType;

public class FlowBuilder {
    private final BiFunction<String, JobType, ? extends JobBuilderImpl> jobBuilderSource;
    private final List<JobBuilder> jobBuilders = new ArrayList<>();
    private final List<JobBuilder> cleanupJobBuilders = new ArrayList<>();

    private FlowBuilder(BiFunction<String, JobType, ? extends JobBuilderImpl> jobBuilderSource) {
        this.jobBuilderSource = jobBuilderSource;
    }

    public static FlowBuilder create() {
        return new FlowBuilder(JobBuilderImpl::new);
    }

    @VisibleForTesting
    public static FlowBuilder create(BiFunction<String, JobType, ? extends JobBuilderImpl> jobBuilderSource) {
        return new FlowBuilder(jobBuilderSource);
    }

    //

    public JobBuilder withJob(@Nonnull TaskletExecutorContext tasklet, String id, JobType jobType) {
        return withJobBuilder(jobBuilderSource.apply(id, jobType)).withTasklet(tasklet);
    }

    public JobBuilder withJob(@Nonnull TaskletV2ExecutorContext tasklet, String id, JobType jobType) {
        return withJobBuilder(jobBuilderSource.apply(id, jobType)).withTaskletV2(tasklet);
    }

    public JobBuilder withJob(@Nonnull SandboxExecutorContext sandboxTask, String id, JobType jobType) {
        return withJobBuilder(jobBuilderSource.apply(id, jobType)).withSandboxTask(sandboxTask);
    }

    public JobBuilder withJob(@Nonnull InternalExecutorContext internal, String id, JobType jobType) {
        return withJobBuilder(jobBuilderSource.apply(id, jobType)).withInternal(internal);
    }

    public JobBuilder withJob(@Nonnull UUID uuid, String id) {
        return withJob(uuid, id, JobType.DEFAULT);
    }

    public JobBuilder withJob(@Nonnull UUID uuid, String id, JobType jobType) {
        return withJob(InternalExecutorContext.of(uuid), id, jobType);
    }

    public Flow build() {
        // Make sure to include all stages defined in registry
        var stageGroups = jobBuilders.stream()
                .map(JobBuilder::getStage)
                .filter(Objects::nonNull)
                .map(Stage::getStageGroup)
                .distinct()
                .collect(Collectors.toList());

        Preconditions.checkState(stageGroups.size() <= 1,
                "All jobs must have same stage group or no stage groups at all, found %s",
                stageGroups.size());

        List<Stage> stages = stageGroups.stream()
                .map(StageGroup::getStages)
                .flatMap(Collection::stream)
                .distinct()
                .sorted()
                .collect(Collectors.toList());

        var jobs = convertJobBuildersToJobs(jobBuilders);
        var cleanupJobs = convertJobBuildersToJobs(cleanupJobBuilders);
        Flow flow = new Flow(this, jobs, cleanupJobs, stages);

        Map<String, Long> duplicatedJob = Stream.of(flow.getJobs(), flow.getCleanupJobs())
                .flatMap(Collection::stream)
                .map(Job::getId)
                .collect(Collectors.groupingBy(Function.identity(), Collectors.counting()));

        List<String> duplicatedIds = duplicatedJob.entrySet().stream()
                .filter(e -> e.getValue() > 1)
                .map(Map.Entry::getKey)
                .collect(Collectors.toList());

        Preconditions.checkState(
                duplicatedIds.isEmpty(),
                "All job IDs must be unique, found duplicate job: %s",
                duplicatedIds
        );

        return flow;
    }

    public List<JobBuilder> getJobBuilders() {
        return jobBuilders;
    }

    public List<JobBuilder> getCleanupJobBuilders() {
        return cleanupJobBuilders;
    }

    private JobBuilder withJobBuilder(JobBuilder jobBuilder) {
        var list = switch (jobBuilder.getJobType()) {
            case DEFAULT -> jobBuilders;
            case CLEANUP -> cleanupJobBuilders;
        };
        list.add(jobBuilder);
        return jobBuilder;
    }

    public JobBuilder getJobBuilder(String jobId) {
        return jobBuilders.stream()
                .filter(j -> jobId.equals(j.getId()))
                .findFirst()
                .orElseThrow(() -> new NoSuchElementException("No job found with id " + jobId));
    }

    private static List<Job> convertJobBuildersToJobs(Collection<JobBuilder> jobBuilders) {
        return convertJobBuildersToJobs(jobBuilders, new IdentityHashMap<>());
    }

    private static List<Job> convertJobBuildersToJobs(
            Collection<JobBuilder> jobBuilders,
            Map<JobBuilder, Job> jobBuilderToJobMap
    ) {
        return jobBuilders.stream()
                .map(job -> convertJobBuilderToJob(job, jobBuilderToJobMap))
                .collect(Collectors.toList());
    }

    private static Set<UpstreamLink<Job>> convertUpstreamJobBuildersToUpstreamJobs(
            Set<UpstreamLink<JobBuilder>> upstreamJobBuilders,
            Map<JobBuilder, Job> jobBuilderToJobMap
    ) {
        return upstreamJobBuilders.stream()
                .map(job ->
                        new UpstreamLink<>(
                                convertJobBuilderToJob(job.getEntity(), jobBuilderToJobMap),
                                job.getType(),
                                job.getStyle()
                        )
                ).collect(Collectors.toSet());
    }

    private static Job convertJobBuilderToJob(
            JobBuilder jobBuilder,
            Map<JobBuilder, Job> jobBuilderToJobMap
    ) {
        return jobBuilderToJobMap.computeIfAbsent(jobBuilder, j -> convertWithIdJobBuilderToJob(j, jobBuilderToJobMap));
    }

    private static Job convertWithIdJobBuilderToJob(
            JobBuilder jobBuilder,
            Map<JobBuilder, Job> jobBuilderToJobMap
    ) {
        var upstreams = convertUpstreamJobBuildersToUpstreamJobs(jobBuilder.getUpstreams(), jobBuilderToJobMap);
        return new Job(jobBuilder, upstreams);
    }

}
