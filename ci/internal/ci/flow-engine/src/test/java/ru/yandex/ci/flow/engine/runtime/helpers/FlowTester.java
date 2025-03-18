package ru.yandex.ci.flow.engine.runtime.helpers;

import java.nio.file.Path;
import java.util.List;
import java.util.Queue;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.atomic.AtomicInteger;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.protobuf.GeneratedMessageV3;
import com.google.protobuf.InvalidProtocolBufferException;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.FlowFullId;
import ru.yandex.ci.core.config.a.validation.AYamlValidationException;
import ru.yandex.ci.core.job.JobResourceType;
import ru.yandex.ci.core.launch.LaunchFlowInfo;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.proto.ProtobufReflectionUtils;
import ru.yandex.ci.core.resources.Resource;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.definition.Flow;
import ru.yandex.ci.flow.engine.definition.stage.Stage;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.FlowProvider;
import ru.yandex.ci.flow.engine.runtime.JobLauncher;
import ru.yandex.ci.flow.engine.runtime.di.model.StoredResourceContainer;
import ru.yandex.ci.flow.engine.runtime.events.FlowEvent;
import ru.yandex.ci.flow.engine.runtime.events.ForceSuccessTriggerEvent;
import ru.yandex.ci.flow.engine.runtime.events.JobExecutorSucceededEvent;
import ru.yandex.ci.flow.engine.runtime.events.JobRunningEvent;
import ru.yandex.ci.flow.engine.runtime.events.JobSucceededEvent;
import ru.yandex.ci.flow.engine.runtime.events.ScheduleChangeEvent;
import ru.yandex.ci.flow.engine.runtime.events.SubscribersSucceededEvent;
import ru.yandex.ci.flow.engine.runtime.events.TriggerEvent;
import ru.yandex.ci.flow.engine.runtime.helpers.TestJobScheduler.TriggeredJob;
import ru.yandex.ci.flow.engine.runtime.helpers.TestJobWaitingScheduler.SchedulerTriggeredJob;
import ru.yandex.ci.flow.engine.runtime.state.FlowStateService;
import ru.yandex.ci.flow.engine.runtime.state.LaunchParameters;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.JobLaunch;
import ru.yandex.ci.flow.engine.runtime.state.model.LaunchInfo;
import ru.yandex.ci.flow.test.TestFlowId;

@Slf4j
@RequiredArgsConstructor
public class FlowTester {
    @Nonnull
    private final FlowStateService flowStateService;
    @Nonnull
    private final TestJobScheduler testJobScheduler;
    @Nonnull
    private final TestJobWaitingScheduler testJobWaitingScheduler;
    @Nonnull
    private final JobLauncher jobLauncher;
    @Nonnull
    private final FlowProvider flowProvider;
    @Nonnull
    private final TestLaunchAutoReleaseDelegate launchAutoReleaseDelegate;
    @Nonnull
    private final FlowTestQueries flowTestQueries;
    @Nonnull
    private final CiDb db;

    private final AtomicInteger launchNumberSeq = new AtomicInteger(0);

    public void reset() {
        testJobScheduler.getTriggeredJobs().clear();
        testJobWaitingScheduler.getTriggeredJobs().clear();
        launchAutoReleaseDelegate.getFlowsWhichUnlockedStage().clear();
        flowProvider.clear();
    }

    public FlowLaunchId runFlowToCompletion(Flow flow) {
        return runFlowToCompletion(flow, null);
    }

    public FlowLaunchId runFlowToCompletion(Flow flow, @Nullable String stageGroupId) {
        var flowId = flowProvider.register(flow, TestFlowId.TEST_PATH);
        return tryRunFlowToCompletion(flowId, stageGroupId);
    }

    public FlowLaunchId runFlowToCompletion(FlowFullId flowId) {
        return tryRunFlowToCompletion(flowId, null);
    }

    public FlowLaunchId tryRunFlowToCompletion(FlowFullId flowId, @Nullable String stageGroupId) {
        try {
            return runFlowToCompletion(flowId, stageGroupId);
        } catch (AYamlValidationException e) {
            throw new RuntimeException(e);
        }
    }

    private FlowLaunchId runFlowToCompletion(
            FlowFullId flowId,
            @Nullable String stageGroupId
    ) throws AYamlValidationException {
        log.info("Running flow: {}", flowId);
        Flow flow = flowProvider.get(flowId);
        if (!flow.getStages().isEmpty()) {
            if (stageGroupId == null) {
                stageGroupId = UUID.randomUUID().toString();
            }

            createStageGroupState(stageGroupId, flow.getStages());
        }

        var processId = flow.getStages().isEmpty()
                ? CiProcessId.ofFlow(flowId)
                : CiProcessId.ofRelease(flowId.getPath(), flowId.getId());
        LaunchId launchId = new LaunchId(processId, launchNumberSeq.incrementAndGet());

        FlowLaunchId flowLaunchId = flowStateService
                .activateLaunch(
                        LaunchParameters.builder()
                                .flowInfo(FlowTestUtils.toFlowInfo(flowId, stageGroupId))
                                .launchId(launchId)
                                .launchInfo(LaunchInfo.of(String.valueOf(launchId.getNumber())))
                                .vcsInfo(FlowTestUtils.VCS_INFO)
                                .projectId("prj")
                                .flow(flow)
                                .triggeredBy(TestData.USER42)
                                .build()
                ).getFlowLaunchId();

        runScheduledJobsToCompletion();
        return flowLaunchId;
    }

    public FlowLaunchId activateLaunch(Flow flow) {
        return activateLaunch(flow, null);
    }

    public FlowLaunchId activateLaunch(Flow flow, @Nullable String stageGroupId) {
        if (!flow.getStages().isEmpty()) {
            if (stageGroupId == null) {
                stageGroupId = UUID.randomUUID().toString();
            }

            createStageGroupState(stageGroupId, flow.getStages());
        }

        var flowId = flowProvider.register(flow, TestFlowId.TEST_PATH);
        LaunchFlowInfo flowInfo = FlowTestUtils.toFlowInfo(flowId, stageGroupId);

        LaunchId launchId = new LaunchId(CiProcessId.ofFlow(flowInfo.getFlowId()), launchNumberSeq.incrementAndGet());

        FlowLaunchEntity flowLaunch = flowStateService.activateLaunch(
                LaunchParameters.builder()
                        .flowInfo(flowInfo)
                        .launchId(launchId)
                        .launchInfo(LaunchInfo.of("" + launchId.getNumber()))
                        .vcsInfo(FlowTestUtils.VCS_INFO)
                        .projectId("prj")
                        .flow(flow)
                        .triggeredBy(TestData.USER42)
                        .build()
        );
        return flowLaunch.getFlowLaunchId();
    }

    public void runScheduledJobToCompletion(String jobId) {
        testJobScheduler.getTriggeredJobs().stream()
                .filter(j -> j.getJobLaunchId().getJobId().equals(jobId))
                .forEach(j -> jobLauncher.launchJob(j.getJobLaunchId(), DummyTmsTaskIdFactory.create()));
    }

    public void runScheduledJobsToCompletion() {
        Queue<TriggeredJob> triggerCommands = testJobScheduler.getTriggeredJobs();
        log.info("Triggered jobs: {}", triggerCommands);

        Queue<SchedulerTriggeredJob> schedulerTriggeredCommands = testJobWaitingScheduler.getTriggeredJobs();
        while (!triggerCommands.isEmpty() || !schedulerTriggeredCommands.isEmpty()) {
            if (!triggerCommands.isEmpty()) {
                TriggeredJob triggeredJob = triggerCommands.poll();
                jobLauncher.launchJob(triggeredJob.getJobLaunchId(), DummyTmsTaskIdFactory.create());
            } else {
                SchedulerTriggeredJob triggeredJob = schedulerTriggeredCommands.poll();
                flowStateService.recalc(
                        triggeredJob.getJobLaunchId().getFlowLaunchId(),
                        new ScheduleChangeEvent(triggeredJob.getJobLaunchId().getJobId())
                );
            }
        }
    }

    public Thread runScheduledJobsToCompletionAsync() {
        Thread thread = new Thread(this::runScheduledJobsToCompletion);
        thread.start();
        return thread;
    }

    public void createStageGroupState(String id, List<Stage> stages) {
        db.currentOrTx(() ->
                db.stageGroup().initStage(id));
    }

    public FlowLaunchEntity triggerJob(FlowLaunchId flowLaunchId, String jobId) {
        return flowStateService.recalc(flowLaunchId, new TriggerEvent(jobId, "user42", false));
    }

    public FlowLaunchEntity forceTriggerJob(FlowLaunchId flowLaunchId, String jobId) {
        return flowStateService.recalc(flowLaunchId, new ForceSuccessTriggerEvent(jobId, "user42"));
    }

    public StoredResourceContainer getProducedResources(FlowLaunchId flowLaunchId, String jobId) {
        return flowTestQueries.getProducedResources(flowLaunchId, jobId);
    }

    public StoredResourceContainer getConsumedResources(FlowLaunchId flowLaunchId, String jobId) {
        return flowTestQueries.getConsumedResources(flowLaunchId, jobId);
    }

    public JobLaunch getJobLastLaunch(FlowLaunchId flowLaunchId, String jobId) {
        return flowTestQueries.getJobLastLaunch(flowLaunchId, jobId);
    }

    public FlowLaunchEntity getFlowLaunch(FlowLaunchId flowLaunchId) {
        return flowTestQueries.getFlowLaunch(flowLaunchId);
    }

    public void saveFlowLaunch(FlowLaunchEntity flowLaunch) {
        db.currentOrTx(() -> db.flowLaunch().save(flowLaunch));
    }

    @SuppressWarnings("unchecked")
    public <T extends GeneratedMessageV3> T getResourceOfType(StoredResourceContainer resources, T message) {
        Resource resource = resources.instantiate(JobResourceType.ofMessage(message));
        try {
            return (T) ProtobufReflectionUtils.merge(resource, message.newBuilderForType()).build();
        } catch (InvalidProtocolBufferException e) {
            throw new RuntimeException("Unable to restore protobuf message from resource " + resource, e);
        }
    }

    public void raiseJobExecuteEventsChain(FlowLaunchId launchId, String jobId) {
        flowStateService.recalc(launchId, new JobRunningEvent(jobId, 1, DummyTmsTaskIdFactory.create()));
        flowStateService.recalc(launchId, new JobExecutorSucceededEvent(jobId, 1));
        flowStateService.recalc(launchId, new SubscribersSucceededEvent(jobId, 1));
        flowStateService.recalc(launchId, new JobSucceededEvent(jobId, 1));
    }

    public FlowLaunchEntity recalcFlowLaunch(FlowLaunchId launchId, FlowEvent flowEvent) {
        return flowStateService.recalc(launchId, flowEvent);
    }

    public Set<FlowLaunchId> getFlowsWhichTriggeredAutoReleases() {
        return launchAutoReleaseDelegate.getFlowsWhichUnlockedStage();
    }

    public FlowFullId register(Flow flow, Path path) {
        return flowProvider.register(flow, path);
    }

    public FlowFullId register(Flow flow, Path path, String flowId) {
        return flowProvider.register(flow, path, flowId);
    }

    public FlowFullId register(Flow flow, FlowFullId flowFullId) {
        return flowProvider.register(flow, flowFullId);
    }
}
