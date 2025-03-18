package ru.yandex.ci.flow.engine.definition.context.impl;

import java.util.Objects;
import java.util.Optional;

import ru.yandex.ci.core.job.JobResource;
import ru.yandex.ci.core.launch.LaunchFlowDescription;
import ru.yandex.ci.core.launch.LaunchPullRequestInfo;
import ru.yandex.ci.core.launch.ReleaseVcsInfo;
import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.model.FlowLaunchContext;
import ru.yandex.ci.util.Overrider;

public class FlowLaunchContextBuilder {

    private FlowLaunchContextBuilder() {
        //
    }

    static FlowLaunchContext createContext(JobContext context) {
        var flowLaunch = context.getFlowLaunch();
        var flowInfo = flowLaunch.getFlowInfo();

        var runtimeInfo = flowInfo.getRuntimeInfo();
        var jobState = context.getJobState();

        var runtimeConfig = Overrider.overrideNullable(
                runtimeInfo.getRuntimeConfig(),
                jobState.getExecutorContext().getJobRuntimeConfig());
        var sandboxOwner = Objects.requireNonNullElse(
                runtimeInfo.getSandboxOwner(),
                runtimeConfig.getSandbox().getOwner());

        var requirementsConfig = Overrider.overrideNullable(
                runtimeInfo.getRequirementsConfig(),
                jobState.getExecutorContext().getRequirements());

        var vcsInfo = flowLaunch.getVcsInfo();
        FlowLaunchContext.Builder builder = FlowLaunchContext.builder()
                .flowStarted(flowLaunch.getCreatedDate())
                .flowId(flowLaunch.getFlowFullId())
                .targetRevision(flowLaunch.getTargetRevision())
                .targetCommit(vcsInfo.getCommit())
                .launchId(flowLaunch.getLaunchId())
                .launchInfo(flowLaunch.getLaunchInfo())
                .flowLaunchId(flowLaunch.getFlowLaunchId())
                .jobId(context.getJobId())
                .jobLaunchNumber(context.getLaunchNumber())
                .jobTitle(jobState.getTitle())
                .jobDescription(jobState.getDescription())
                .yavTokenUid(runtimeInfo.getYavTokenUidOrThrow())
                .sandboxOwner(sandboxOwner)
                .runtimeConfig(runtimeConfig)
                .requirements(requirementsConfig)
                .previousRevision(flowLaunch.getPreviousRevision())
                .triggeredBy(flowLaunch.getTriggeredBy())
                .title(flowLaunch.getTitle())
                .selectedBranch(vcsInfo.getSelectedBranch())
                .rollbackToVersion(flowInfo.getRollbackToVersion())
                .projectId(flowLaunch.getProjectId())
                .jobAttemptsConfig(jobState.getRetry());

        Optional.ofNullable(flowInfo.getFlowVars())
                .map(JobResource::getData)
                .ifPresent(builder::flowVars);

        Optional.ofNullable(flowInfo.getFlowDescription())
                .map(LaunchFlowDescription::getFlowType)
                .ifPresent(builder::flowType);

        ReleaseVcsInfo vcsReleaseInfo = vcsInfo.getReleaseVcsInfo();
        if (vcsReleaseInfo != null) {
            builder.vcsReleaseInfo(vcsReleaseInfo);
        }

        LaunchPullRequestInfo pullRequestInfo = vcsInfo.getPullRequestInfo();
        if (pullRequestInfo != null) {
            builder.launchPullRequestInfo(pullRequestInfo);
        }

        var lastLaunch = jobState.getLaunchByNumber(context.getLaunchNumber());
        if (lastLaunch.getTriggeredBy() != null) {
            builder.jobTriggeredBy(lastLaunch.getTriggeredBy());
        }
        builder.jobStarted(lastLaunch.getFirstStatusChange().getDate());

        var manualTrigger = jobState.getManualTriggerModifications();
        if (manualTrigger != null) {
            builder.manualTriggeredBy(manualTrigger.getModifiedBy());
            builder.manualTriggeredAt(manualTrigger.getTimestamp());
        }

        return builder.build();
    }
}
