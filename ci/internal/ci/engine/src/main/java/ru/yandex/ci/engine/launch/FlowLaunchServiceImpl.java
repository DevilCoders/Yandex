package ru.yandex.ci.engine.launch;

import java.util.HashMap;
import java.util.Objects;
import java.util.Set;
import java.util.function.Function;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import lombok.AllArgsConstructor;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import ru.yandex.ci.api.proto.Common;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.a.model.AYamlConfig;
import ru.yandex.ci.core.config.a.model.FlowWithFlowVars;
import ru.yandex.ci.core.config.a.validation.AYamlValidationException;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.job.JobInstance;
import ru.yandex.ci.core.launch.FlowReference;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchFlowInfo;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.launch.LaunchState;
import ru.yandex.ci.engine.config.ConfigBundle;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.definition.Flow;
import ru.yandex.ci.flow.engine.definition.job.Job;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.state.FlowStateService;
import ru.yandex.ci.flow.engine.runtime.state.LaunchParameters;
import ru.yandex.ci.flow.engine.runtime.state.model.DelegatedOutputResources;
import ru.yandex.ci.flow.engine.runtime.state.model.JobState;
import ru.yandex.ci.flow.engine.runtime.state.model.JobType;
import ru.yandex.ci.flow.engine.runtime.state.model.LaunchInfo;
import ru.yandex.ci.flow.engine.runtime.state.model.StoredStage;

@AllArgsConstructor
public class FlowLaunchServiceImpl implements FlowLaunchService {
    private static final Logger log = LoggerFactory.getLogger(FlowLaunchServiceImpl.class);

    @Nonnull
    private final FlowFactory flowFactory;

    @Nonnull
    private final CiDb db;

    @Nonnull
    private final FlowStateService flowStateService;

    @Override
    public FlowLaunchId launchFlow(Launch launch, ConfigBundle bundle) throws AYamlValidationException {
        log.info("Launching {}", launch.getLaunchId());

        var flowInfo = launch.getFlowInfo();
        var flowDesc = flowInfo.getFlowDescription();

        // TODO проверить, что flowType есть всегда
        var flowReference = flowDesc != null && flowDesc.getFlowType() != Common.FlowType.FT_DEFAULT
                ? FlowReference.of(flowInfo.getFlowId().getId(), flowDesc.getFlowType())
                : null;

        var customizedCfg = FlowCustomizedConfig.builder();
        customizedCfg.flowReference(flowReference);
        if (flowInfo.getFlowVars() != null) {
            customizedCfg.flowVars(flowInfo.getFlowVars().getData());
        }

        var processId = launch.getLaunchId().getProcessId();
        var aYamlConfig = bundle.getValidAYamlConfig();

        Flow flow = flowFactory.create(
                processId,
                aYamlConfig,
                bundle.getTaskConfigs(),
                customizedCfg.build());
        log.info("Created flow with {} jobs and {} cleanup jobs",
                flow.getJobs().size(), flow.getCleanupJobs().size());

        var parameters = LaunchParameters.builder()
                .launchId(launch.getLaunchId())
                .projectId(launch.getProject())
                .vcsInfo(launch.getVcsInfo())
                .flowInfo(flowInfo)
                .launchInfo(LaunchInfo.of(launch.getVersion()))
                .flow(flow)
                .triggeredBy(launch.getTriggeredBy())
                .title(launch.getTitle());

        fillRollbackFlow(
                processId,
                aYamlConfig,
                flowInfo,
                flowDesc != null ? flowDesc.getRollbackUsingLaunch() : null,
                flowReference,
                flow,
                parameters);

        var flowLaunch = flowStateService.activateLaunch(parameters.build());
        var flowLaunchId = flowLaunch.getFlowLaunchId();

        log.info("Flow launched: {}", flowLaunchId);
        return flowLaunchId;
    }

    private void fillRollbackFlow(
            CiProcessId processId,
            AYamlConfig yamlConfig,
            LaunchFlowInfo launchFlowInfo,
            @Nullable LaunchId rollbackUsingLaunch,
            @Nullable FlowReference rollbackFlowReference,
            Flow flow,
            LaunchParameters.Builder parameterBuilder
    ) {
        if (rollbackUsingLaunch == null) {
            Preconditions.checkState(rollbackFlowReference == null ||
                            rollbackFlowReference.getFlowType() != Common.FlowType.FT_ROLLBACK,
                    "Rollback flow cannot be started without reference to rollback launch");
            return; // ---
        }

        Preconditions.checkState(rollbackFlowReference != null,
                "rollbackUsingLaunch parameter cannot be used with default flow type");
        Preconditions.checkState(rollbackFlowReference.getFlowType() == Common.FlowType.FT_ROLLBACK,
                "rollbackUsingLaunch parameter cannot be used for flow type %s", rollbackFlowReference.getFlowType());

        log.info("Configuring rollback flow: {}, rollback reference: {}", processId, rollbackFlowReference);

        var release = yamlConfig.getCi().getRelease(processId.getSubId());

        var originalLaunch = validateRollbackLaunch(db, rollbackUsingLaunch);
        var originalFlowId = originalLaunch.getFlowInfo().getFlowId().getId();
        if (launchFlowInfo.getRollbackToVersion() == null) {
            parameterBuilder.flowInfo(launchFlowInfo.toBuilder()
                    .rollbackToVersion(originalLaunch.getVersion())
                    .build());
        }

        var flowId = rollbackFlowReference.getFlowId();
        for (FlowWithFlowVars rollbackFlow : release.getRollbackFlows()) {
            if (Objects.equals(rollbackFlow.getFlow(), flowId)) {
                Preconditions.checkState(rollbackFlow.acceptFlow(originalFlowId),
                        "rollbackLaunch %s is invalid, rollback flow accepts only flows %s, found %s",
                        rollbackUsingLaunch, rollbackFlow.getAcceptFlows(), originalFlowId);
                break; // ---
            }
        }

        Preconditions.checkState(originalLaunch.getFlowLaunchId() != null,
                "Internal error. FlowLaunchId cannot be empty");
        var rollbackFlowLaunch = db.flowLaunch().get(FlowLaunchId.of(originalLaunch.getFlowLaunchId()));

        Set<String> nonRollbackStages = rollbackFlowLaunch.getStages().stream()
                .filter(stage -> !stage.isRollback())
                .map(StoredStage::getId)
                .collect(Collectors.toSet());

        if (nonRollbackStages.isEmpty()) {
            log.info("No matched stages detected, no resources will be copied");
            return; // ----
        }

        // Check jobs in rollback flow
        // Make sure to NOT copy resources if job with same ID already exists but not skipped
        var jobs = flow.getJobs().stream()
                .collect(Collectors.toMap(Job::getId, Function.identity()));
        var jobResources = new HashMap<String, DelegatedOutputResources>();
        var multiplyByMapping = new HashMap<String, String>();
        for (JobState rollbackJob : rollbackFlowLaunch.getJobs().values()) {
            if (rollbackJob.getJobType() != JobType.DEFAULT ||
                    rollbackJob.getStage() == null ||
                    !nonRollbackStages.contains(rollbackJob.getStage().getId())) {
                log.info("Skip configuring job [{}]", rollbackJob.getJobId());
                continue; // ---
            }

            var lastLaunch = rollbackJob.getLastLaunch();
            if (lastLaunch == null) { // Nothing to copy if job never launched
                continue; // ---
            }

            var jobId = rollbackJob.getJobId();
            var currentJob = jobs.get(jobId);
            if (currentJob != null) {
                if (currentJob.getSkippedByMode() == null) {
                    log.info("Do not add resource for job [{}], job already exists and it's not skipped", jobId);
                    continue; // ---
                }
                log.info("Job [{}] exists in skipped mode {}", jobId, currentJob.getSkippedByMode());
            } else {
                log.info("Job [{}] does not exists", jobId);
                if (rollbackJob.getJobTemplateId() != null) {
                    // This is a generated job from multiply/by in ignored stage and such jobs won't exists in our flow
                    // Mix all resources from generated jobs into the single one
                    multiplyByMapping.put(jobId, rollbackJob.getJobTemplateId());
                    Preconditions.checkState(!jobId.equals(rollbackJob.getJobTemplateId()),
                            "Internal error, jobId and jobTemplateId are the same");
                }
            }
            var resourceRef = lastLaunch.getProducedResources();
            var jobOutput = JobInstance.Id.of(
                    rollbackFlowLaunch.getFlowLaunchId().asString(),
                    jobId,
                    lastLaunch.getNumber()
            );
            log.info("Include resource ref to job [{}]: resources = {}, output = {}",
                    jobId, resourceRef, jobOutput);
            jobResources.put(jobId, DelegatedOutputResources.of(resourceRef, jobOutput));
        }

        // Merge multiply/by jobs to one
        for (var e : multiplyByMapping.entrySet()) {
            var jobId = e.getKey();
            var jobTemplateId = e.getValue();
            var resource = jobResources.remove(jobId);
            if (resource != null) {
                var templateResource = jobResources.get(jobTemplateId);
                if (templateResource != null) {
                    jobResources.put(jobTemplateId, templateResource.mergeWith(resource.getResourceRef()));
                    log.info("Merge resources from job [{}] with [{}]", jobId, jobTemplateId);
                }
            }
        }

        parameterBuilder.jobResources(jobResources);
    }

    @Override
    public void cancelFlow(FlowLaunchId flowLaunchId) {
        db.currentOrTx(() -> {
            flowStateService.disableJobsInLaunchGracefully(flowLaunchId, jobState -> true, false, false);
            flowStateService.disableLaunchGracefully(flowLaunchId, false);
        });
    }

    @Override
    public void cancelJobs(FlowLaunchId flowLaunchId, JobType jobType) {
        db.currentOrTx(() ->
                flowStateService.disableJobsInLaunchGracefully(
                        flowLaunchId, jobState -> jobState.getJobType() == jobType, false, false));
    }

    @Override
    public void cleanupFlow(FlowLaunchId flowLaunchId) {
        db.currentOrTx(() -> flowStateService.cleanupLaunch(flowLaunchId));
    }

    public static Launch validateRollbackLaunch(CiMainDb db, LaunchId rollbackUsingLaunch) {
        var launch = db.currentOrReadOnly(() -> db.launches().get(rollbackUsingLaunch));

        var rollbackDescription = launch.getFlowInfo().getFlowDescription();
        if (rollbackDescription != null && rollbackDescription.getFlowType() == Common.FlowType.FT_ROLLBACK) {
            throw new IllegalStateException("rollbackLaunch %s must not be another %s flow"
                    .formatted(rollbackUsingLaunch, Common.FlowType.FT_ROLLBACK));
        }
        if (launch.getFlowLaunchId() == null) {
            throw new IllegalStateException("rollbackLaunch flow %s has no flow launch"
                    .formatted(rollbackUsingLaunch));
        }
        if (launch.getStatus() != LaunchState.Status.SUCCESS) {
            throw new IllegalStateException("rollbackLaunch flow %s must be %s but it's %s"
                    .formatted(rollbackUsingLaunch, LaunchState.Status.SUCCESS, launch.getStatus()));
        }
        return launch;
    }
}
