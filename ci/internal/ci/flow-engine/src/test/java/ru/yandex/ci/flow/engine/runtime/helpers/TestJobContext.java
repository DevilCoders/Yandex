package ru.yandex.ci.flow.engine.runtime.helpers;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.List;
import java.util.Objects;
import java.util.UUID;
import java.util.function.Consumer;

import org.mockito.stubbing.Answer;

import ru.yandex.ci.core.config.FlowFullId;
import ru.yandex.ci.core.internal.InternalExecutorContext;
import ru.yandex.ci.core.resources.Resource;
import ru.yandex.ci.flow.engine.definition.context.JobProgressContext;
import ru.yandex.ci.flow.engine.definition.context.impl.JobContextImpl;
import ru.yandex.ci.flow.engine.definition.context.impl.JobProgressContextImpl;
import ru.yandex.ci.flow.engine.definition.context.impl.JobResourcesContextImpl;
import ru.yandex.ci.flow.engine.definition.context.impl.UpstreamResourcesCollector;
import ru.yandex.ci.flow.engine.definition.job.JobProgress;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.di.ResourceService;
import ru.yandex.ci.flow.engine.runtime.di.model.ResourceContainer;
import ru.yandex.ci.flow.engine.runtime.state.FlowStateService;
import ru.yandex.ci.flow.engine.runtime.state.JobProgressService;
import ru.yandex.ci.flow.engine.runtime.state.model.ExecutorContext;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.JobLaunch;
import ru.yandex.ci.flow.engine.runtime.state.model.JobState;
import ru.yandex.ci.flow.engine.runtime.state.model.TaskBadge;
import ru.yandex.ci.flow.engine.source_code.SourceCodeService;

import static org.mockito.Mockito.any;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

public class TestJobContext extends JobContextImpl {

    private static final String DEFAULT_JOB_ID = "1";
    private static final String DEFAULT_FULL_JOB_ID = "1:1:1";
    public static final FlowFullId DEFAULT_FLOW_ID = new FlowFullId("dir", "testFlowId");
    public static final FlowLaunchId DEFAULT_FLOW_LAUNCH_ID = FlowLaunchId.of("111111111111111111111111");

    private String jobId = DEFAULT_JOB_ID;
    private String fullJobId = DEFAULT_FULL_JOB_ID;

    private final JobProgressContextMock jobProgressContext = new JobProgressContextMock();
    private final List<Resource> resources = new ArrayList<>();

    public TestJobContext(FlowLaunchEntity flowLaunchEntity) {
        this(flowLaunchEntity, getJobStateInternal());
    }

    public TestJobContext(FlowLaunchEntity flowLaunchEntity, JobState jobState) {
        super(JobContextImpl.builder()
                .upstreamResourcesCollector(mock(UpstreamResourcesCollector.class))
                .jobProgressService(mock(JobProgressService.class))
                .flowLaunch(flowLaunchEntity)
                .jobState(jobState)
                .flowStateService(mock(FlowStateService.class))
                .resourceService(mock(ResourceService.class))
                .sourceCodeService(mock(SourceCodeService.class))
                .buildInternal());
        var service = ((JobResourcesContextImpl) resources()).getResourceService();
        when(service.loadResources(any()))
                .thenAnswer((Answer<ResourceContainer>) invocation -> new ResourceContainer(resources));
    }

    // Make sure to call this method before accessing any resource
    public void registerConsumedResource(Resource resource) {
        resources.add(resource);
    }

    @Override
    public String getJobId() {
        return jobId;
    }

    public void setJobId(String jobId) {
        this.jobId = jobId;
    }

    @Override
    public String getFullJobId() {
        return fullJobId;
    }

    public void setFullJobId(String fullJobId) {
        this.fullJobId = fullJobId;
    }

    public Progress getLastProgress() {
        JobProgress progress = jobProgressContext.currentProgress;
        return new Progress(
                progress.getText(), progress.getRatio(), new ArrayList<>(progress.getTaskStates().values())
        );
    }

    private static JobState getJobStateInternal() {
        var state = new JobState();
        state.addLaunch(mock(JobLaunch.class));
        state.setExecutorContext(new ExecutorContext(
                InternalExecutorContext.of(UUID.randomUUID()),
                null,
                null,
                null,
                null,
                null));
        return state;
    }

    public static class Progress {
        private final String statusText;
        private final Float progressRatio;
        private final List<TaskBadge> taskStates;

        public Progress(String statusText, Float progressRatio, List<TaskBadge> taskStates) {
            this.statusText = statusText;
            this.progressRatio = progressRatio;
            this.taskStates = List.copyOf(Objects.requireNonNullElse(taskStates, Collections.emptyList()));
        }

        public String getStatusText() {
            return statusText;
        }

        public Float getProgressRatio() {
            return progressRatio;
        }

        /**
         * Never returns null.
         */
        public List<TaskBadge> getTaskStates() {
            return taskStates;
        }
    }

    @Override
    public JobProgressContext progress() {
        return jobProgressContext;
    }

    private static class JobProgressContextMock implements JobProgressContext {
        private final JobProgress currentProgress = new JobProgress();

        @Override
        public void update(Consumer<JobProgressContextImpl.ProgressBuilder> callback) {
            callback.accept(JobProgressContextImpl.builder(currentProgress));
        }

        @Override
        public TaskBadge getTaskState(String module) {
            return getTaskState(0, module);
        }

        @Override
        public TaskBadge getTaskState(long index, String module) {
            return currentProgress.getTaskStates().get(module + index);
        }

        @Override
        public Collection<TaskBadge> getTaskStates() {
            return currentProgress.getTaskStates().values();
        }

        @Override
        public void updateTaskState(TaskBadge taskBadge) {
            update(progressBuilder -> progressBuilder.setTaskBadge(taskBadge));
        }
    }
}
