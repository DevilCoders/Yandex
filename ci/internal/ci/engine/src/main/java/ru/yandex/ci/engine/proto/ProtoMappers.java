package ru.yandex.ci.engine.proto;

import java.nio.file.Path;
import java.time.DayOfWeek;
import java.time.Instant;
import java.util.Collection;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Optional;
import java.util.Set;
import java.util.function.Consumer;
import java.util.function.Function;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import com.google.common.collect.Iterables;
import com.google.protobuf.Int64Value;
import com.google.protobuf.StringValue;
import com.google.protobuf.Timestamp;
import lombok.Value;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.api.internal.frontend.flow.FrontendFlowApi;
import ru.yandex.ci.api.internal.frontend.flow.FrontendFlowApi.JobState;
import ru.yandex.ci.api.internal.frontend.flow.FrontendFlowApi.Point;
import ru.yandex.ci.api.internal.frontend.flow.FrontendFlowApi.Prompt;
import ru.yandex.ci.api.internal.frontend.flow.FrontendFlowApi.SchedulerConstraints;
import ru.yandex.ci.api.internal.frontend.flow.FrontendFlowApi.StatusChangeInfo;
import ru.yandex.ci.api.internal.frontend.flow.oncommit.FrontendOnCommitFlowLaunchApi;
import ru.yandex.ci.api.internal.frontend.release.FrontendTimelineApi;
import ru.yandex.ci.api.proto.Common;
import ru.yandex.ci.api.proto.Common.DisplacedBy;
import ru.yandex.ci.api.proto.Common.StatusChangeWrapper;
import ru.yandex.ci.api.proto.Common.Version;
import ru.yandex.ci.client.abc.AbcServiceInfo;
import ru.yandex.ci.common.proto.ProtoConverter;
import ru.yandex.ci.core.abc.AbcServiceEntity;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.ArcCommitUtils;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.CommitId;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.arc.branch.Branch;
import ru.yandex.ci.core.arc.branch.BranchInfo;
import ru.yandex.ci.core.arc.branch.ReleaseBranchId;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.FlowFullId;
import ru.yandex.ci.core.config.VirtualCiProcessId.VirtualType;
import ru.yandex.ci.core.config.a.AYamlService;
import ru.yandex.ci.core.config.a.model.FlowVarsUi;
import ru.yandex.ci.core.config.a.model.JobMultiplyConfig;
import ru.yandex.ci.core.config.a.model.LargeAutostartConfig;
import ru.yandex.ci.core.config.a.model.NativeBuildConfig;
import ru.yandex.ci.core.db.model.AutoReleaseSettingsHistory;
import ru.yandex.ci.core.discovery.DiscoveredCommitState;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchDisplacedBy;
import ru.yandex.ci.core.launch.LaunchDisplacementChange;
import ru.yandex.ci.core.launch.LaunchFlowInfo;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.launch.LaunchPullRequestInfo;
import ru.yandex.ci.core.launch.LaunchState;
import ru.yandex.ci.core.launch.LaunchTableFilter;
import ru.yandex.ci.core.pr.PullRequestDiffSet;
import ru.yandex.ci.core.pr.PullRequestVcsInfo;
import ru.yandex.ci.core.project.ActionConfigState;
import ru.yandex.ci.core.project.ProcessIdBranch;
import ru.yandex.ci.core.project.ProjectCounters;
import ru.yandex.ci.core.project.ReleaseConfigState;
import ru.yandex.ci.core.proto.CiCoreProtoMappers;
import ru.yandex.ci.core.resources.Resource;
import ru.yandex.ci.core.timeline.Offset;
import ru.yandex.ci.core.timeline.ReleaseCommit;
import ru.yandex.ci.core.timeline.TimelineBranchItem;
import ru.yandex.ci.core.timeline.TimelineBranchItemByUpdateDate;
import ru.yandex.ci.core.timeline.TimelineItem;
import ru.yandex.ci.engine.launch.auto.AutoReleaseState;
import ru.yandex.ci.flow.engine.definition.common.JobSchedulerConstraintEntity;
import ru.yandex.ci.flow.engine.definition.common.ManualTriggerModifications;
import ru.yandex.ci.flow.engine.definition.common.SchedulerConstraintModifications;
import ru.yandex.ci.flow.engine.definition.common.SchedulerIntervalEntity;
import ru.yandex.ci.flow.engine.definition.common.TypeOfSchedulerConstraint;
import ru.yandex.ci.flow.engine.definition.common.UpstreamLink;
import ru.yandex.ci.flow.engine.definition.common.WeekSchedulerConstraintEntity;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.di.model.AbstractResourceContainer;
import ru.yandex.ci.flow.engine.runtime.di.model.StoredResourceId;
import ru.yandex.ci.flow.engine.runtime.state.StageGroupState;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntityForVersion;
import ru.yandex.ci.flow.engine.runtime.state.model.JobLaunch;
import ru.yandex.ci.flow.engine.runtime.state.model.JobType;
import ru.yandex.ci.flow.engine.runtime.state.model.StagedLaunchStatistics;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChange;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChangeType;
import ru.yandex.ci.flow.engine.runtime.state.model.StoredStage;
import ru.yandex.ci.flow.engine.runtime.state.model.TaskBadge;
import ru.yandex.ci.storage.core.CheckOuterClass;
import ru.yandex.ci.util.OffsetResults;

@Slf4j
public class ProtoMappers {

    private ProtoMappers() {
    }

    public static FlowLaunchId toFlowLaunchId(Common.FlowLaunchId proto) {
        return FlowLaunchId.of(proto.getId());
    }

    public static Common.FlowLaunchId toProtoFlowLaunchId(FlowLaunchId id) {
        return Common.FlowLaunchId.newBuilder()
                .setId(id.asString())
                .build();
    }

    public static FrontendFlowApi.LaunchState toProtoLaunchState(
            FlowLaunchEntity flowLaunch, Launch launch, Common.StagesState stagesState
    ) {
        Map<String, Set<String>> childrenMap = computeChildrenMap(flowLaunch.getJobs().values());

        List<JobState> jobs = flowLaunch.getJobs().values().stream()
                .map(state -> toProtoJobState(state, childrenMap))
                .collect(Collectors.toList());

        var builder = FrontendFlowApi.LaunchState.newBuilder();
        flowLaunch.getStageGroupId()
                .ifPresent(builder::setStageGroupId);
        if (flowLaunch.getProjectId() != null) {
            builder.setProjectId(flowLaunch.getProjectId());
        }

        Optional.ofNullable(flowLaunch.getTitle())
                .ifPresent(builder::setTitle);

        Optional.ofNullable(flowLaunch.getFlowInfo())
                .map(ProtoMappers::toProtoLaunchFlowInfo)
                .ifPresent(builder::setFlowDescription);

        builder.setId(flowLaunch.getFlowLaunchId().asString())
                .setFlowId(flowLaunch.getFlowFullId().asString())
                .setStateVersion(flowLaunch.getStateVersion())
                .setIsDisabled(flowLaunch.isDisabled())
                .setIsDisablingGracefully(flowLaunch.isDisablingGracefully())
                .setIsShouldIgnoreUninterruptableStages(flowLaunch.shouldIgnoreUninterruptibleStages())
                .setIsStaged(flowLaunch.isStaged())
                .addAllJobs(jobs)
                .setConfigRevision(flowLaunch.getFlowInfo().getConfigRevision().getCommitId())
                .setConfigPath(flowLaunch.getLaunchId().getProcessId().getPath().toString())
                .setStagesState(stagesState)
                .setCancelable(!launch.getStatus().isTerminal() && VirtualType.of(launch.getProcessId()) == null)
                .setProcessId(toProtoProcessId(launch.getProcessId()));

        return builder.build();
    }

    public static Common.LaunchId toProtoLaunchId(LaunchId launchId) {
        return Common.LaunchId.newBuilder()
                .setProcessId(toProtoProcessId(launchId.getProcessId()))
                .setNumber(launchId.getNumber())
                .build();
    }

    private static Common.ProcessId toProtoProcessId(CiProcessId processId) {
        var builder = Common.ProcessId.newBuilder();
        return switch (processId.getType()) {
            case RELEASE -> builder.setReleaseProcessId(toProtoReleaseProcessId(processId)).build();
            case FLOW -> builder.setFlowProcessId(toProtoFlowProcessId(processId)).build();
            case SYSTEM -> throw new IllegalStateException("Unsupported type: " + processId.getType());
        };
    }

    public static Common.StagesState toProtoStagesState(
            FlowLaunchEntity flowLaunch,
            Function<Set<String>, Map<String, StageGroupState>> stageGroupsLoader,
            Function<Set<FlowLaunchId>, Map<FlowLaunchId, FlowLaunchEntityForVersion>> launchIdLoader) {
        var result = toProtoStagesStates(List.of(flowLaunch), stageGroupsLoader, launchIdLoader)
                .get(flowLaunch.getId());
        Preconditions.checkState(result != null, "Internal error. Unable to find stage for %s", flowLaunch.getId());
        return result;
    }

    public static Map<FlowLaunchEntity.Id, Common.StagesState> toProtoStagesStates(
            Collection<FlowLaunchEntity> flowLaunches,
            Function<Set<String>, Map<String, StageGroupState>> stageGroupsLoader,
            Function<Set<FlowLaunchId>, Map<FlowLaunchId, FlowLaunchEntityForVersion>> launchIdLoader) {

        var stageGroupMap = stageGroupsLoader.apply(findStageGroups(flowLaunches));

        var blockedStageMap = new HashMap<FlowLaunchEntity, Map<String, FlowLaunchId>>(flowLaunches.size());
        var allFlowLaunchIds = new HashSet<FlowLaunchId>();
        for (var launch : flowLaunches) {
            var stageGroup = launch.getStageGroupId().map(stageGroupMap::get).orElse(null);
            var blockedStages = findBlockedStages(launch.getFlowLaunchId(), stageGroup);
            blockedStageMap.put(launch, blockedStages);
            allFlowLaunchIds.addAll(blockedStages.values());
        }

        var launchIdMap = launchIdLoader.apply(allFlowLaunchIds);

        var result = new LinkedHashMap<FlowLaunchEntity.Id, Common.StagesState>(flowLaunches.size());
        for (var flowLaunch : flowLaunches) {
            var stageGroup = flowLaunch.getStageGroupId().map(stageGroupMap::get).orElse(null);
            var stageIdToStatus = computeStageStatuses(flowLaunch, stageGroup);

            var blockedMap = blockedStageMap.get(flowLaunch).entrySet().stream()
                    .collect(Collectors.toMap(
                            Map.Entry::getKey,
                            e -> LaunchAndVersion.of(launchIdMap.get(e.getValue()))));

            result.put(flowLaunch.getId(),
                    ProtoMappers.toProtoStagesState(flowLaunch, stageIdToStatus, blockedMap));
        }
        return result;
    }


    private static Set<String> findStageGroups(Collection<FlowLaunchEntity> flowLaunches) {
        return flowLaunches.stream()
                .map(FlowLaunchEntity::getStageGroupId)
                .filter(Optional::isPresent)
                .map(Optional::get)
                .collect(Collectors.toSet());
    }

    static Map<String, FlowLaunchId> findBlockedStages(FlowLaunchId flowLaunchId,
                                                       @Nullable StageGroupState stageGroupState) {
        if (stageGroupState == null) {
            return Map.of();
        }
        var item = stageGroupState.getQueueItem(flowLaunchId);
        String desiredStageId = item
                .map(StageGroupState.QueueItem::getLockIntent)
                .map(StageGroupState.LockIntent::getDesiredStageId)
                .orElse(null);
        if (desiredStageId == null) {
            return Map.of();
        }

        var blockedStages = stageGroupState.getQueue()
                .stream()
                .filter(it -> it.getAcquiredStageIds().contains(desiredStageId))
                .findFirst()
                .map(StageGroupState.QueueItem::getFlowLaunchId)
                .map(launchId -> Map.of(desiredStageId, launchId))
                .orElseGet(Map::of);
        if (blockedStages.isEmpty()) {
            var otherItem = stageGroupState.getNextQueueItem(item.orElseThrow())
                    .or(() -> stageGroupState.getPreviousQueueItem(item.orElseThrow()));
            if (otherItem.isPresent()) {
                return Map.of(desiredStageId, otherItem.get().getFlowLaunchId());
            }
        }
        return blockedStages;
    }

    private static Map<String, StageStatusAndColumns> computeStageStatuses(
            FlowLaunchEntity flowLaunch,
            @Nullable StageGroupState stageGroupState) {
        Map<String, StageStatusAndColumns> stageIdToStatus = new HashMap<>();

        if (stageGroupState != null) {
            stageGroupState.getQueueItem(flowLaunch.getFlowLaunchId())
                    .map(StageGroupState.QueueItem::getAcquiredStageIds)
                    .orElseGet(Set::of)
                    .forEach(stageId -> stageIdToStatus.put(stageId,
                            new StageStatusAndColumns(Common.StageState.StageStatus.ACQUIRED)));

            String desiredStageId = stageGroupState.getQueueItem(flowLaunch.getFlowLaunchId())
                    .map(StageGroupState.QueueItem::getDesiredStageId)
                    .orElse(null);

            for (var jobState : flowLaunch.getJobs().values()) {
                if (jobState.getStage() == null) {
                    continue;
                }
                var stageId = jobState.getStage().getId();
                if (stageIdToStatus.containsKey(stageId)) {
                    continue;
                }
                boolean jobDesiresStage = stageId.equals(desiredStageId);
                if (!jobDesiresStage && jobState.getLastStatusChangeType() != null) {
                    // status is RELEASED, cause status ACQUIRED was computed earlier
                    stageIdToStatus.put(stageId,
                            new StageStatusAndColumns(Common.StageState.StageStatus.RELEASED));
                }
            }

            for (StoredStage stage : flowLaunch.getStages()) {
                stageIdToStatus.putIfAbsent(stage.getId(),
                        new StageStatusAndColumns(Common.StageState.StageStatus.NOT_ACQUIRED));
            }
        }

        for (var jobState : flowLaunch.getJobs().values()) {
            if (jobState.getStage() == null) {
                continue;
            }
            var stageId = jobState.getStage().getId();
            var stage = stageIdToStatus.get(stageId);
            if (stage != null) {
                stage.differentPositions.add(jobState.getPosition().getX());
            }
        }

        return stageIdToStatus;
    }


    public static Common.StagesState toProtoStagesState(
            FlowLaunchEntity flowLaunch,
            Map<String, StageStatusAndColumns> stageIdToStatus,
            Map<String, LaunchAndVersion> blockedStageIdToLaunch) {

        var stats = StagedLaunchStatistics.fromLaunch(flowLaunch);

        var builder = Common.StagesState.newBuilder();
        flowLaunch.getStages()
                .stream()
                .map(storedStage -> {
                    var stage = storedStage.getId();
                    var status = stageIdToStatus.getOrDefault(
                            stage,
                            new StageStatusAndColumns(Common.StageState.StageStatus.NOT_ACQUIRED)
                    );
                    var stageState = Common.StageState.newBuilder()
                            .setId(stage)
                            .setTitle(Objects.requireNonNullElse(storedStage.getTitle(), "No Title"))
                            .setStatus(status.status)
                            .setTotalColumns(status.differentPositions.size());

                    var launchBlocksNextStage = blockedStageIdToLaunch.get(stage);
                    if (launchBlocksNextStage != null) {
                        var launch = ProtoMappers.toReleaseLaunchId(launchBlocksNextStage.launchId);
                        var version = ProtoMappers.toProtoVersion(launchBlocksNextStage.version);
                        stageState.setBlockedByRelease(launch);
                        stageState.setBlockedByVersion(
                                Common.BlockedBy.newBuilder()
                                        .setId(launch)
                                        .setVersion(version)
                        );
                    }

                    var stat = stats.getStats().get(stage);
                    if (stat != null) {
                        var stageStatistics = stat.getStageStatistics();
                        long started = stageStatistics.getStarted();
                        long finished = stageStatistics.getFinished();
                        if (started > 0 && finished > 0) {
                            stageState.setStarted(toProtoTimestamp(Instant.ofEpochMilli(started)));
                            stageState.setFinished(toProtoTimestamp(Instant.ofEpochMilli(finished)));
                        }

                        var launchStatistics = stat.getLaunchStatistics();
                        launchStatistics.forEach((type, count) -> stageState.addJobsStatusBuilder()
                                .setType(toProtoStatusChangeType2(type))
                                .setCount(count));
                    }

                    return stageState.build();
                })
                .forEach(builder::addStates);
        return builder.build();
    }

    public static JobState toProtoJobState(ru.yandex.ci.flow.engine.runtime.state.model.JobState state,
                                           Map<String, Set<String>> childrenMap) {
        JobState.Builder builder = JobState.newBuilder();

        Optional.ofNullable(state.getTitle())
                .ifPresent(builder::setTitle);
        Optional.ofNullable(state.getDescription())
                .ifPresent(builder::setDescription);

        builder.setId(state.getJobId())
                .setIsOutdated(state.isOutdated())
                .setIsReadyToRun(state.isReadyToRun())
                .setIsManualTrigger(state.isManualTrigger());

        if (state.isManualTrigger() &&
                state.getManualTriggerPrompt() != null &&
                state.getManualTriggerPrompt().getQuestion() != null) {
            builder.setManualTriggerPrompt(
                    Prompt.newBuilder()
                            .setQuestion(state.getManualTriggerPrompt().getQuestion())
            );
        }

        Optional.ofNullable(state.getJobSchedulerConstraint())
                .map(ProtoMappers::toProtoSchedulerConstraints)
                .ifPresent(builder::setSchedulerConstraints);

        builder.setIsEnableWaitingForScheduler(state.isEnableJobSchedulerConstraint())
                .setCanRunWhen(JobState.CanRunWhen.valueOf(state.getCanRunWhen().name()))
                .setIsVisible(state.isVisible())
                .setIsDisabled(state.isDisabled())
                .setIsProducesResources(state.isProducesResources());

        Optional.ofNullable(state.getConditionalRunExpression())
                .ifPresent(builder::setConditionalRunExpression);

        builder.setConditionalSkip(state.isConditionalSkip());

        Optional.ofNullable(state.getJobTemplateId())
                .ifPresent(builder::setTemplateId);

        Optional.ofNullable(state.getMultiply())
                .map(JobMultiplyConfig::getBy)
                .ifPresent(builder::setMultiplyByExpression);

        if (state.getPosition() != null) {
            builder.setPosition(
                    Point.newBuilder()
                            .setX(state.getPosition().getX())
                            .setY(state.getPosition().getY())
            );
        }

        state.getUpstreams().stream()
                .map(u -> FrontendFlowApi.JobUpstream.newBuilder()
                        .setJobId(u.getEntity())
                        .setStyle(FrontendFlowApi.JobUpstream.Style.valueOf(u.getStyle().name()))
                        .setType(FrontendFlowApi.JobUpstream.Type.valueOf(u.getType().name()))
                ).forEach(builder::addUpstreams);

        childrenMap.getOrDefault(state.getJobId(), Collections.emptySet()).stream()
                .map(s -> FrontendFlowApi.JobDownstream.newBuilder().setJobId(s))
                .forEach(builder::addDownstreams);

        ManualTriggerModifications mtm = state.getManualTriggerModifications();
        if (mtm != null) {
            builder.setManualTriggerModifications(
                    FrontendFlowApi.ModificationLogEntry.newBuilder()
                            .setModifiedBy(mtm.getModifiedBy())
                            .setTimestamp(toProtoTimestamp(mtm.getTimestamp()))
                            .build()
            );
        }
        SchedulerConstraintModifications scm = state.getSchedulerConstraintModifications();
        if (scm != null) {
            builder.setManualTriggerModifications(
                    FrontendFlowApi.ModificationLogEntry.newBuilder()
                            .setModifiedBy(scm.getModifiedBy())
                            .setTimestamp(toProtoTimestamp(scm.getTimestamp()))
                            .build()
            );
        }

        if (state.getStage() != null) {
            builder.setStage(
                    FrontendFlowApi.JobStage.newBuilder()
                            .setId(state.getStage().getId())
                            .build()
            );
        }

        JobLaunch lastLaunch = state.getLastLaunch();

        if (lastLaunch != null) {
            var lastType = lastLaunch.getLastStatusChangeType();
            builder.setStatus(toProtoStatusChangeType(lastType))
                    .setStatus2(toProtoStatusChangeType2(lastType))
                    .setLaunchNumber(lastLaunch.getNumber())
                    .setIsCanInterrupt(true);

            Optional.ofNullable(lastLaunch.getTriggeredBy())
                    .ifPresent(builder::setTriggeredBy);
            Optional.ofNullable(lastLaunch.getInterruptedBy())
                    .ifPresent(builder::setInterruptedBy);
            Optional.ofNullable(lastLaunch.getForceSuccessTriggeredBy())
                    .ifPresent(value -> {
                        builder.setForceSuccessTriggeredBy(value);
                        builder.setIsForceSuccess(true);
                    });
            Optional.ofNullable(lastLaunch.getStatusText())
                    .ifPresent(builder::setStatusText);

            Optional.ofNullable(lastLaunch.getTotalProgress())
                    .ifPresent(builder::setTotalProgress);

            lastLaunch.getTaskStates().stream()
                    .map(ProtoMappers::toProtoTaskState)
                    .forEach(builder::addTaskStates);

            List<StatusChange> statusHistory = lastLaunch.getStatusHistory();
            StatusChange runningStatusChange = statusHistory.stream()
                    .filter(s -> s.getType() == StatusChangeType.RUNNING)
                    .findFirst()
                    .orElse(null);

            if (runningStatusChange != null) {
                builder.setStartDate(toProtoTimestamp(runningStatusChange.getDate()));
            } else {
                builder.setStartDate(toProtoTimestamp(statusHistory.get(0).getDate()));
            }

            StatusChange lastStatusChange = statusHistory.get(statusHistory.size() - 1);
            if (lastStatusChange.getType() == StatusChangeType.SUCCESSFUL
                    || lastStatusChange.getType() == StatusChangeType.FAILED
                    || lastStatusChange.getType() == StatusChangeType.EXPECTED_FAILED) {
                builder.setEndDate(toProtoTimestamp(lastStatusChange.getDate()));
            }

            var scheduleTime = lastLaunch.getScheduleTime();
            if (scheduleTime != null) {
                builder.setScheduleDate(toProtoTimestamp(scheduleTime));
            }
        }

        Optional.ofNullable(state.getJobType())
                .map(ProtoMappers::toProtoJobType)
                .ifPresent(builder::setJobType);

        return builder.build();
    }

    public static JobState.JobType toProtoJobType(JobType jobType) {
        return JobState.JobType.valueOf(jobType.name());
    }

    public static FrontendFlowApi.TaskState toProtoTaskState(TaskBadge taskBadge) {
        FrontendFlowApi.TaskState.Builder builder = FrontendFlowApi.TaskState.newBuilder()
                .setStatus(FrontendFlowApi.TaskState.Status.valueOf(taskBadge.getStatus().name()));

        Optional.ofNullable(taskBadge.getModule())
                .ifPresent(builder::setModuleName);
        Optional.ofNullable(taskBadge.getUrl())
                .ifPresent(builder::setUrl);
        Optional.ofNullable(taskBadge.getText())
                .ifPresent(builder::setText);
        Optional.ofNullable(taskBadge.getProgress())
                .ifPresent(builder::setProgress);
        builder.setPrimary(taskBadge.isPrimary());

        return builder.build();
    }

    private static SchedulerConstraints toProtoSchedulerConstraints(JobSchedulerConstraintEntity constraintEntity) {
        SchedulerConstraints.Builder builder = SchedulerConstraints.newBuilder();
        var weekConstraints = constraintEntity.getWeekConstraints();

        addConstraintType(weekConstraints.get(TypeOfSchedulerConstraint.WORK), builder::addWork);
        addConstraintType(weekConstraints.get(TypeOfSchedulerConstraint.PRE_HOLIDAY), builder::addPreHoliday);
        addConstraintType(weekConstraints.get(TypeOfSchedulerConstraint.HOLIDAY), builder::addHoliday);

        return builder.build();
    }

    private static void addConstraintType(@Nullable WeekSchedulerConstraintEntity weekConstraint,
                                          Consumer<SchedulerConstraints.DayOfWeekInterval> setter) {
        if (weekConstraint != null) {
            for (var dayOfWeekConstraint : weekConstraint.getAllowedDayOfWeekIntervals().entrySet()) {
                DayOfWeek day = dayOfWeekConstraint.getKey();
                SchedulerIntervalEntity interval = dayOfWeekConstraint.getValue();
                SchedulerConstraints.DayOfWeekInterval dayOfWeekInterval =
                        SchedulerConstraints.DayOfWeekInterval.newBuilder()
                                .setType(FrontendFlowApi.DayOfWeek.valueOf(day.name()))
                                .setAllowed(toProtoDayInterval(interval))
                                .build();

                setter.accept(dayOfWeekInterval);
            }
        }
    }

    private static SchedulerConstraints.DayInterval toProtoDayInterval(SchedulerIntervalEntity interval) {
        return SchedulerConstraints.DayInterval.newBuilder()
                .setMinutesFrom(interval.getMinutesFrom())
                .setMinutesTo(interval.getMinutesTo())
                .build();
    }

    public static Timestamp toProtoTimestamp(Instant instant) {
        return ProtoConverter.convert(instant);
    }

    public static Instant fromProtoTimestamp(Timestamp timestamp) {
        return ProtoConverter.convert(timestamp);
    }

    public static FrontendFlowApi.JobLaunchListItem toProtoJobLaunchListItem(JobLaunch jobLaunch) {
        var lastType = jobLaunch.getLastStatusChangeType();
        return FrontendFlowApi.JobLaunchListItem.newBuilder()
                .setNumber(jobLaunch.getNumber())
                .setStatus(toProtoStatusChangeType(lastType))
                .setStatus2(toProtoStatusChangeType2(lastType))
                .setLaunchDate(toProtoTimestamp(jobLaunch.getFirstStatusChange().getDate()))
                .build();
    }

    public static StatusChangeInfo toProtoStatusChangeType(StatusChange sc) {
        return StatusChangeInfo.newBuilder()
                .setDate(toProtoTimestamp(sc.getDate()))
                .setType(toProtoStatusChangeType(sc.getType()))
                .setType2(toProtoStatusChangeType2(sc.getType()))
                .build();
    }

    public static StatusChangeInfo.StatusChangeType toProtoStatusChangeType(StatusChangeType statusChangeType) {
        return StatusChangeInfo.StatusChangeType.valueOf(statusChangeType.name());
    }

    public static StatusChangeWrapper.StatusChangeType toProtoStatusChangeType2(
            @Nullable StatusChangeType statusChangeType) {
        if (statusChangeType == null) {
            return StatusChangeWrapper.StatusChangeType.NO_STATUS;
        } else {
            return StatusChangeWrapper.StatusChangeType.valueOf(statusChangeType.name());
        }
    }

    public static List<FrontendFlowApi.JobLaunchResourceViewModel> toProtoJobLaunchResourceViewModels(
            AbstractResourceContainer container) {

        return container.getAllById().entrySet().stream()
                .map(entry -> toProtoJobLaunchResourceViewModel(entry.getKey(), entry.getValue()))
                .collect(Collectors.toList());
    }

    private static FrontendFlowApi.JobLaunchResourceViewModel toProtoJobLaunchResourceViewModel(StoredResourceId id,
                                                                                                Resource resource) {
        FrontendFlowApi.JobLaunchResourceViewModel.Builder builder =
                FrontendFlowApi.JobLaunchResourceViewModel.newBuilder()
                        .setId(id.asString())
                        .setClassName(resource.getResourceType().getMessageName());

        builder.setId(resource.renderTitle());
        builder.setObject(resource.renderResource());
        return builder.build();
    }

    public static LaunchId toLaunchId(Common.ReleaseLaunchId releaseLaunchId) {
        return new LaunchId(toCiProcessId(releaseLaunchId.getReleaseProcessId()), releaseLaunchId.getNumber());
    }

    public static CiProcessId toCiProcessId(Common.ReleaseProcessId releaseProcessId) {
        return CiProcessId.ofRelease(AYamlService.dirToConfigPath(releaseProcessId.getDir()),
                releaseProcessId.getId());
    }

    public static ReleaseBranchId toReleaseBranchId(Common.BranchId branchId) {
        return ReleaseBranchId.of(
                toCiProcessId(branchId.getReleaseProcessId()),
                ArcBranch.ofBranchName(branchId.getBranch())
        );
    }

    public static CiProcessId toCiProcessId(Common.FlowProcessId flowProcessId) {
        return CiProcessId.ofFlow(AYamlService.dirToConfigPath(flowProcessId.getDir()), flowProcessId.getId());
    }

    public static FlowFullId toFlowFullId(Common.FlowProcessId flowProcessId) {
        return CiCoreProtoMappers.toFlowFullId(flowProcessId);
    }

    public static Common.ReleaseLaunch toProtoReleaseLaunch(Launch launch) {
        return toProtoReleaseLaunch(launch, null, false);
    }

    public static Common.ReleaseLaunch toProtoReleaseLaunch(
            Launch launch,
            @Nullable Common.StagesState stagesState,
            boolean restartable
    ) {
        var vcsInfo = launch.getVcsInfo();

        Common.ReleaseLaunch.Builder releaseLaunchBuilder = Common.ReleaseLaunch.newBuilder()
                .setId(toReleaseLaunchId(launch.getLaunchId()))
                .setTitle(launch.getTitle())
                .setRevision(toProtoOrderedArcRevision(vcsInfo.getRevision()));

        Optional.ofNullable(vcsInfo.getPreviousRevision())
                .map(ProtoMappers::toProtoOrderedArcRevision)
                .ifPresent(releaseLaunchBuilder::setPreviousRevision);

        releaseLaunchBuilder.setCommitCount(vcsInfo.getCommitCount());
        releaseLaunchBuilder.setTriggeredBy(launch.getTriggeredBy());
        Optional.ofNullable(launch.getCancelledBy())
                .ifPresent(releaseLaunchBuilder::setCancelledBy);
        Optional.ofNullable(launch.getCancelledReason())
                .ifPresent(releaseLaunchBuilder::setCancelledReason);

        Optional.ofNullable(launch.getFlowInfo())
                .map(ProtoMappers::toProtoLaunchFlowInfo)
                .ifPresent(releaseLaunchBuilder::setFlowDescription);

        releaseLaunchBuilder.setCreated(toProtoTimestamp(launch.getCreated()));
        LaunchState state = launch.getState();

        state.getStarted().map(ProtoMappers::toProtoTimestamp).ifPresent(releaseLaunchBuilder::setStarted);
        state.getFinished().map(ProtoMappers::toProtoTimestamp).ifPresent(releaseLaunchBuilder::setFinished);
        state.getFlowLaunchId()
                .map(FlowLaunchId::of)
                .map(ProtoMappers::toProtoFlowLaunchId)
                .ifPresent(releaseLaunchBuilder::setFlowLaunchId);

        releaseLaunchBuilder.setStatus(toProtoLaunchStatus(state.getStatus()));
        releaseLaunchBuilder.setCancelable(!state.getStatus().isTerminal());
        // TODO: is null-value possible?
        if (state.getText() != null) {
            releaseLaunchBuilder.setStatusText(state.getText());
        } else {
            // TODO: delete if null-value is never used
            log.warn("status text is null: launchId: {}", launch.getId());
        }
        releaseLaunchBuilder.setCancelledReleasesCount(launch.getCancelledReleases().size());
        releaseLaunchBuilder.setDisplacedReleasesCount(launch.getDisplacedReleases().size());
        releaseLaunchBuilder.setDisplaced(launch.isDisplaced());
        Optional.ofNullable(launch.getDisplacedBy())
                .map(ProtoMappers::convertDisplacedBy)
                .ifPresent(releaseLaunchBuilder::setDisplacedByVersion);
        releaseLaunchBuilder.setHasDisplacement(!launch.getStatus().isTerminal() &&
                Boolean.TRUE.equals(launch.getHasDisplacement()));

        launch.getDisplacementChanges().stream()
                .map(ProtoMappers::toProtoDisplacementChange)
                .forEach(releaseLaunchBuilder::addDisplacementChanges);

        Optional.ofNullable(stagesState)
                .ifPresent(releaseLaunchBuilder::setStagesState);
        releaseLaunchBuilder.setVersion(toProtoVersion(launch.getVersion()));
        releaseLaunchBuilder.setRestartable(restartable);

        Optional.ofNullable(launch.getLaunchReason())
                .ifPresent(releaseLaunchBuilder::setLaunchReason);

        return releaseLaunchBuilder.build();
    }

    public static DisplacedBy convertDisplacedBy(LaunchDisplacedBy launchDisplacedBy) {
        return DisplacedBy.newBuilder()
                .setId(toReleaseLaunchId(LaunchId.fromKey(launchDisplacedBy.getId())))
                .setVersion(toProtoVersion(launchDisplacedBy.getVersion()))
                .build();
    }

    public static Version toProtoVersion(ru.yandex.ci.core.launch.versioning.Version version) {
        Version.Builder builder = Version.newBuilder()
                .setFull(version.asString())
                .setMajor(version.getMajor());

        if (version.getMinor() != null) {
            builder.setMinor(version.getMinor());
        }
        return builder.build();
    }

    public static Optional<ru.yandex.ci.core.launch.versioning.Version> fromProtoVersion(Version version) {
        if (!version.getMajor().isEmpty()) {
            if (!version.getMinor().isEmpty()) {
                return Optional.of(ru.yandex.ci.core.launch.versioning.Version.majorMinor(
                        version.getMajor(), version.getMinor()));
            } else {
                return Optional.of(ru.yandex.ci.core.launch.versioning.Version.major(version.getMajor()));
            }
        } else if (!version.getFull().isEmpty()) {
            return Optional.of(ru.yandex.ci.core.launch.versioning.Version.fromAsString(version.getFull()));
        } else {
            return Optional.empty();
        }
    }


    public static Common.DisplacementChange toProtoDisplacementChange(LaunchDisplacementChange change) {
        return Common.DisplacementChange.newBuilder()
                .setState(change.getState())
                .setChangedBy(change.getChangedBy())
                .setChangedAt(toProtoTimestamp(change.getChangedAt()))
                .build();
    }

    public LaunchDisplacementChange toDisplacementChange(Common.DisplacementChange change) {
        return LaunchDisplacementChange.of(change.getState(), change.getChangedBy(),
                fromProtoTimestamp(change.getChangedAt()));
    }

    public static Common.LaunchStatus toProtoLaunchStatus(LaunchState.Status status) {
        if (status == LaunchState.Status.SUCCESS) {
            return Common.LaunchStatus.FINISHED;
        }
        return Common.LaunchStatus.valueOf(status.name());
    }

    public static FrontendOnCommitFlowLaunchApi.FlowLaunchStatus toProtoFlowLaunchStatus(LaunchState.Status status) {
        if (status == LaunchState.Status.WAITING_FOR_STAGE) {
            throw new IllegalArgumentException(status.name());
        }
        return FrontendOnCommitFlowLaunchApi.FlowLaunchStatus.valueOf(status.name());
    }

    public static LaunchState.Status toLaunchStatus(Common.LaunchStatus status) {
        if (status == Common.LaunchStatus.FINISHED) {
            return LaunchState.Status.SUCCESS;
        }
        return LaunchState.Status.valueOf(status.name());
    }

    public static LaunchState.Status toLaunchStatus(FrontendOnCommitFlowLaunchApi.FlowLaunchStatus status) {
        return LaunchState.Status.valueOf(status.name());
    }

    public static Common.ReleaseLaunchId toReleaseLaunchId(LaunchId launchId) {
        Preconditions.checkArgument(launchId.getProcessId().getType() == CiProcessId.Type.RELEASE,
                "launchId process type [%s] must be %s",
                launchId.getProcessId().getType(), CiProcessId.Type.RELEASE);
        return Common.ReleaseLaunchId.newBuilder()
                .setReleaseProcessId(toProtoReleaseProcessId(launchId.getProcessId()))
                .setNumber(launchId.getNumber())
                .build();
    }

    public static Common.ReleaseProcessId toProtoReleaseProcessId(CiProcessId processId) {
        Preconditions.checkArgument(processId.getType() == CiProcessId.Type.RELEASE);
        return Common.ReleaseProcessId.newBuilder()
                .setDir(processId.getDir())
                .setId(processId.getSubId())
                .build();
    }

    public static Common.FlowProcessId toProtoFlowProcessId(CiProcessId processId, String flow) {
        return Common.FlowProcessId.newBuilder()
                .setDir(processId.getDir())
                .setId(flow)
                .build();
    }

    public static Common.FlowProcessId toProtoFlowProcessId(CiProcessId processId) {
        return CiCoreProtoMappers.toProtoFlowProcessId(processId);
    }

    public static Common.FlowProcessId toProtoFlowProcessId(FlowFullId flowFullId) {
        return CiCoreProtoMappers.toProtoFlowProcessId(flowFullId);
    }

    public static Common.Commit toProtoCommit(OrderedArcRevision revision, ArcCommit commit) {
        Common.Commit.Builder builder = Common.Commit.newBuilder()
                .setRevision(toProtoOrderedArcRevision(revision))
                .setAuthor(commit.getAuthor())
                .setDate(ProtoMappers.toProtoTimestamp(commit.getCreateTime()))
                .addAllIssues(ArcCommitUtils.parseTickets(commit.getMessage()));

        Optional<Integer> requestId = ArcCommitUtils.parsePullRequestId(commit.getMessage());
        if (requestId.isPresent()) {
            builder.setPullRequestId(requestId.get());
            builder.setMessage(ArcCommitUtils.cleanupMessage(commit.getMessage()));
        } else {
            builder.setMessage(commit.getMessage());
        }

        return builder.build();
    }

    public static Common.CommitDiscoveryState toCommitDiscoveryStateManual(DiscoveredCommitState state) {
        var result = Common.CommitDiscoveryState.newBuilder();
        result.setDiscovered(state.isManualDiscovery());
        Optional.ofNullable(state.getManualDiscoveryBy())
                .ifPresent(result::setDiscoveredBy);
        Optional.ofNullable(state.getManualDiscoveryAt())
                .map(ProtoMappers::toProtoTimestamp)
                .ifPresent(result::setDiscoveredAt);
        return result.build();
    }

    public static Common.ReleaseCommit toProtoReleaseCommit(ReleaseCommit releaseCommit) {
        Common.ReleaseCommit.Builder releaseCommitBuilder = Common.ReleaseCommit.newBuilder();

        releaseCommit.getCancelledReleases().stream()
                .sorted(Comparator.comparing((Launch l) -> l.getLaunchId().getNumber()).reversed())
                .map(ProtoMappers::toProtoReleaseLaunch)
                .forEach(releaseCommitBuilder::addCancelledReleases);

        releaseCommit.getBranches().stream()
                .map(ArcBranch::asString)
                .forEach(releaseCommitBuilder::addBranches);

        releaseCommitBuilder.setCommit(toProtoCommit(releaseCommit.getRevision(), releaseCommit.getCommit()));

        Optional.ofNullable(releaseCommit.getDiscoveredState())
                .map(ProtoMappers::toCommitDiscoveryStateManual)
                .ifPresent(releaseCommitBuilder::setManualDiscovery);

        return releaseCommitBuilder.build();
    }

    public static Common.ArcCommit toProtoArcCommit(ArcCommit commit) {
        return Common.ArcCommit.newBuilder()
                .setHash(commit.getCommitId())
                .setAuthor(commit.getAuthor())
                .setMessage(commit.getMessage())
                .setDate(ProtoMappers.toProtoTimestamp(commit.getCreateTime()))
                .addAllIssues(ArcCommitUtils.parseTickets(commit.getMessage()))
                .build();
    }

    public static Common.OrderedArcRevision toProtoOrderedArcRevision(OrderedArcRevision revision) {
        return CiCoreProtoMappers.toProtoOrderedArcRevision(revision);
    }

    public static boolean hasOrderedArcRevision(Common.OrderedArcRevision revision) {
        return CiCoreProtoMappers.hasOrderedArcRevision(revision);
    }

    public static OrderedArcRevision toOrderedArcRevision(Common.OrderedArcRevision revision) {
        return CiCoreProtoMappers.toOrderedArcRevision(revision);
    }

    public static Common.LocalizedName toProtoLocalizedName(AbcServiceInfo.LocalizedName name) {
        return Common.LocalizedName.newBuilder()
                .setRu(name.getRu())
                .setEn(name.getEn())
                .build();
    }

    public static ArcRevision toArcRevision(Common.CommitId commitId) {
        return ArcRevision.of(commitId.getCommitId());
    }

    public static Common.CommitId toCommitId(CommitId commitId) {
        return Common.CommitId.newBuilder()
                .setCommitId(commitId.getCommitId())
                .build();
    }

    public static Common.Project toProtoProject(AbcServiceEntity serviceInfo) {
        return Common.Project.newBuilder()
                .setId(serviceInfo.getSlug())
                .setAbcService(toProtoAbcService(serviceInfo))
                .build();
    }

    public static Common.AbcService toProtoAbcService(AbcServiceEntity serviceInfo) {
        return Common.AbcService.newBuilder()
                .setSlug(serviceInfo.getSlug())
                .setUrl(serviceInfo.getUrl())
                .setName(ProtoMappers.toProtoLocalizedName(serviceInfo.getName()))
                .setDescription(ProtoMappers.toProtoLocalizedName(serviceInfo.getDescription()))
                .build();
    }

    public static Common.ReleaseState toProtoReleaseState(
            CiProcessId processId,
            ReleaseConfigState releaseState,
            AutoReleaseState autoReleaseState,
            @Nullable OffsetResults<Branch> branches
    ) {
        return toProtoReleaseState(processId, releaseState, autoReleaseState, branches, ProjectCounters.empty());
    }

    public static Common.ReleaseState toProtoReleaseState(
            CiProcessId processId,
            ReleaseConfigState releaseState,
            AutoReleaseState autoReleaseState,
            @Nullable OffsetResults<Branch> branches,
            ProjectCounters projectCounters
    ) {
        Common.ReleaseState.Builder builder = Common.ReleaseState.newBuilder()
                .setId(ProtoMappers.toProtoReleaseProcessId(processId))
                .setTitle(releaseState.getTitle())
                .setAuto(toProtoAutoReleaseState(autoReleaseState))
                .setReleaseBranchesEnabled(releaseState.isReleaseBranchesEnabled())
                .setReleaseFromTrunkAllowed(!releaseState.isReleaseFromTrunkForbidden())
                .setDefaultConfigFromBranch(releaseState.isDefaultConfigFromBranch());
        Optional.ofNullable(releaseState.getDescription())
                .ifPresent(builder::setDescription);

        // we return trunk for DEVTOOLSUI-2033
        builder.addBranches(
                Common.Branch.newBuilder()
                        .setName(ArcBranch.trunk().asString())
                        .addAllLaunchStatusCounter(
                                toProtoLaunchStatusCounters(
                                        projectCounters,
                                        ProcessIdBranch.of(processId, ArcBranch.trunk().asString())
                                )
                        )
                        .build()
        );

        if (branches != null) {
            branches.items().stream()
                    .map(branch -> {
                        var processIdBranch = ProcessIdBranch.of(branch.getItem());
                        var branchBuilder = toProtoBranchBuilder(branch);
                        branchBuilder.addAllLaunchStatusCounter(
                                toProtoLaunchStatusCounters(projectCounters, processIdBranch)
                        );
                        return branchBuilder.build();
                    })
                    .forEach(builder::addBranches);
            if (branches.hasMore()) {
                Branch last = Iterables.getLast(branches.items());
                Common.BranchOffset branchOffset = ProtoMappers.toProtoBranchOffset(
                        TimelineBranchItemByUpdateDate.Offset.of(last.getItem())
                );
                builder.setBranchesNext(branchOffset);
            }
        }

        return builder.build();
    }

    public static List<Common.LaunchStatusCounter> toProtoLaunchStatusCounters(
            ProjectCounters projectCounters,
            ProcessIdBranch processIdBranch
    ) {
        return projectCounters.getCount(processIdBranch)
                .entrySet()
                .stream()
                .sorted(Map.Entry.comparingByKey())
                .map(it -> Common.LaunchStatusCounter.newBuilder()
                        .setStatus(toProtoLaunchStatus(it.getKey()))
                        .setCount(it.getValue().intValue())
                        .build()
                )
                .toList();
    }

    public static Common.AutoReleaseState toProtoAutoReleaseState(AutoReleaseState autoReleaseState) {
        Common.AutoReleaseState.Builder result = Common.AutoReleaseState.newBuilder()
                .setEnabled(autoReleaseState.isEnabledInAnyConfig())
                .setEditable(autoReleaseState.isEditable());
        if (autoReleaseState.getLatestSettings() != null) {
            result.setLastManualAction(
                    toProtoLastManualAction(autoReleaseState.getLatestSettings())
            );
        }
        return result.build();
    }

    public static Common.AutoReleaseState.LastManualAction toProtoLastManualAction(
            AutoReleaseSettingsHistory lastManualAction
    ) {
        var builder = Common.AutoReleaseState.LastManualAction.newBuilder()
                .setEnabled(lastManualAction.isEnabled())
                .setLogin(lastManualAction.getLogin())
                .setDate(ProtoMappers.toProtoTimestamp(
                        lastManualAction.getId().getTimestamp()
                ));
        if (lastManualAction.getMessage() != null) {
            builder.setMessage(lastManualAction.getMessage());
        }
        return builder.build();
    }

    public static LaunchTableFilter toLaunchTableFilter(
            FrontendOnCommitFlowLaunchApi.GetFlowLaunchesRequest request
    ) {
        var builder = LaunchTableFilter.builder();
        if (request.hasPinned()) {
            builder.pinned(request.getPinned().getValue());
        }
        if (request.hasBranch()) {
            builder.branch(request.getBranch().getValue());
        }
        builder.tags(request.getTagsList());

        if (!request.getFlowStatusesList().isEmpty()) {
            builder.statuses(
                    request.getFlowStatusesList().stream()
                            .map(ProtoMappers::toLaunchStatus)
                            .collect(Collectors.toList())
            );
        } else {
            builder.statuses(
                    request.getStatusesList().stream()
                            .map(ProtoMappers::toLaunchStatus)
                            .collect(Collectors.toList())
            );
        }
        builder.sortBy(request.getSortBy());
        builder.sortDirection(request.getSortDirection());
        return builder.build();
    }

    public static FrontendOnCommitFlowLaunchApi.FlowLaunch toProtoFlowLaunch(
            Launch launch, @Nullable PullRequestDiffSet diffSet
    ) {
        var builder = FrontendOnCommitFlowLaunchApi.FlowLaunch.newBuilder();

        builder.setFlowProcessId(toProtoFlowProcessId(launch.getProcessId()))
                .setNumber(launch.getLaunchId().getNumber())
                .setTitle(launch.getTitle())
                .setTriggeredBy(launch.getTriggeredBy())
                .setRevisionHash(launch.getVcsInfo().getRevision().getCommitId())
                .setBranch(toOnCommitFlowBranch(launch, diffSet))
                .setTriggeredBy(launch.getTriggeredBy())
                .setCreated(toProtoTimestamp(launch.getCreated()));

        Optional.ofNullable(launch.getStarted())
                .map(ProtoMappers::toProtoTimestamp)
                .ifPresent(builder::setStarted);

        Optional.ofNullable(launch.getFinished())
                .map(ProtoMappers::toProtoTimestamp)
                .ifPresent(builder::setFinished);

        builder.setStatus(toProtoLaunchStatus(launch.getStatus()));
        builder.setLaunchStatus(toProtoFlowLaunchStatus(launch.getStatus()));
        // TODO: is null-value possible?
        if (launch.getStatusText() != null) {
            builder.setStatusText(launch.getStatusText());
        } else {
            // TODO: delete if null-value is never used
            log.warn("status text is null: launchId: {}", launch.getId());
        }
        builder.setCancelable(!launch.getStatus().isTerminal() && VirtualType.of(launch.getProcessId()) == null);

        if (launch.getCancelledBy() != null) {
            builder.setCancelledBy(launch.getCancelledBy());
        }
        if (launch.getCancelledReason() != null) {
            builder.setCancelledReason(launch.getCancelledReason());
        }
        builder.setPinned(launch.isPinned())
                .addAllTags(launch.getTags());

        if (launch.getFlowLaunchId() != null) {
            builder.setFlowLaunchId(toProtoFlowLaunchId(FlowLaunchId.of(
                    launch.getFlowLaunchId()
            )));
        }
        Optional.ofNullable(launch.getFlowInfo())
                .map(ProtoMappers::toProtoLaunchFlowInfo)
                .ifPresent(builder::setFlowDescription);

        return builder.build();
    }

    public static FrontendOnCommitFlowLaunchApi.Branch toOnCommitFlowBranch(
            Launch launch, @Nullable PullRequestDiffSet diffSet
    ) {
        ArcBranch branch = launch.getVcsInfo().getSelectedBranch();

        var builder = FrontendOnCommitFlowLaunchApi.Branch.newBuilder()
                .setName(branch.asString());
        if (branch.isPr()) {
            LaunchPullRequestInfo pullRequestInfo = launch.getVcsInfo().getPullRequestInfo();
            Preconditions.checkState(pullRequestInfo != null);
            var pullRequestProto = FrontendOnCommitFlowLaunchApi.PullRequest.newBuilder()
                    .setPullRequestId(branch.getPullRequestId())
                    .setVcsInfo(toProtoPullRequestVcsInfo(
                            pullRequestInfo.getVcsInfo()
                    ));
            if (diffSet != null) {
                pullRequestProto.setSummary(diffSet.getSummary());
                Optional.ofNullable(diffSet.getDescription())
                        .ifPresent(pullRequestProto::setDescription);
            }
            builder.setPullRequest(pullRequestProto);
        }
        return builder.build();
    }

    private static Common.PullRequestVcsInfo toProtoPullRequestVcsInfo(PullRequestVcsInfo prVcsInfo) {
        Common.PullRequestVcsInfo.Builder builder = Common.PullRequestVcsInfo.newBuilder()
                .setMergeRevisionHash(prVcsInfo.getMergeRevision().getCommitId())
                .setUpstreamRevisionHash(prVcsInfo.getUpstreamRevision().getCommitId())
                .setUpstreamBranch(prVcsInfo.getUpstreamBranch().asString())
                .setFeatureRevisionHash(prVcsInfo.getFeatureRevision().getCommitId());
        if (prVcsInfo.getFeatureBranch() != null) {
            builder.setFeatureBranch(StringValue.of(
                    prVcsInfo.getFeatureBranch().asString()
            ));
        }
        return builder.build();
    }

    public static Offset toTimelineOffset(FrontendTimelineApi.TimelineOffset offset) {
        return Offset.of(offset.getRevisionNumber(), offset.getItemNumber());
    }

    public static FrontendTimelineApi.TimelineOffset toProtoTimelineOffset(Offset offset) {
        return FrontendTimelineApi.TimelineOffset.newBuilder()
                .setRevisionNumber(offset.getRevisionNumber())
                .setItemNumber(offset.getItemNumber())
                .build();
    }

    public static Common.TimelineItem toProtoTimelineItem(
            TimelineItem item,
            Map<Launch.Id, Common.StagesState> stageMap,
            Map<Launch.Id, Launch> lastBranchLaunches,
            boolean restartable
    ) {
        var result = Common.TimelineItem.newBuilder();

        @Nullable
        var launch = item.getLaunch();
        @Nullable
        var branch = item.getBranch();
        if (launch != null) {
            result.setRelease(toProtoReleaseLaunch(launch, stageMap.get(launch.getId()), restartable));
        } else if (branch != null) {
            result.setTimelineBranch(toProtoTimelineBranch(branch, lastBranchLaunches));
        } else {
            throw new IllegalArgumentException("expected branch or release item, got " + item);
        }

        var branchName = Optional.ofNullable(item.getShowInBranch())
                .orElseGet(() -> ArcBranch.trunk().asString());
        result.setShowInBranch(branchName);
        return result.build();
    }

    public static Common.Branch.Builder toProtoBranchBuilder(Branch branch) {
        BranchInfo info = branch.getInfo();
        TimelineBranchItem item = branch.getItem();
        int cancelled = item.getState().getCancelledBranchLaunches().size();

        return Common.Branch.newBuilder()
                .setName(info.getArcBranch().asString())
                .setBaseRevisionHash(info.getBaseRevision().getCommitId())
                .setCreated(toProtoTimestamp(info.getCreated()))
                .setCreatedBy(info.getCreatedBy())

                .setTrunkCommitsCount(item.getVcsInfo().getTrunkCommitCount())
                .setBranchCommitsCount(item.getVcsInfo().getBranchCommitCount())
                .setActiveLaunchesCount(item.getState().getActiveLaunches().size())
                .setCompletedLaunchesCount(item.getState().getCompletedLaunches().size())
                .setCancelledLaunchesCount(cancelled);
    }

    public static Common.Branch toProtoBranch(Branch branch) {
        return toProtoBranchBuilder(branch).build();
    }

    public static Common.TimelineBranch toProtoTimelineBranch(Branch branch,
                                                              Map<Launch.Id, Launch> lastBranchLaunches) {
        var branchState = branch.getState();
        var builder = Common.TimelineBranch.newBuilder()
                .setFreeBranchCommitsCount(branchState.getFreeCommits())
                .setBranch(toProtoBranch(branch));
        if (branchState.getLastLaunchNumber() > -1) {
            var launchId = Launch.Id.of(branch.getProcessId().asString(), branchState.getLastLaunchNumber());
            if (lastBranchLaunches.containsKey(launchId)) {
                builder.setLastBranchRelease(toProtoReleaseLaunch(lastBranchLaunches.get(launchId)));
            }
        }

        return builder.build();
    }

    public static TimelineBranchItemByUpdateDate.Offset toBranchOffset(Common.BranchOffset offset) {
        return new TimelineBranchItemByUpdateDate.Offset(
                fromProtoTimestamp(offset.getUpdated()),
                ArcBranch.ofBranchName(offset.getBranchName())
        );
    }

    public static Common.BranchOffset toProtoBranchOffset(TimelineBranchItemByUpdateDate.Offset offset) {
        return Common.BranchOffset.newBuilder()
                .setBranchName(offset.getBranch().asString())
                .setUpdated(toProtoTimestamp(offset.getUpdateDate()))
                .build();
    }

    public static <T> Common.Offset toProtoOffset(OffsetResults<T> offsetResults) {
        Common.Offset.Builder builder = Common.Offset.newBuilder()
                .setHasMore(offsetResults.hasMore());
        if (offsetResults.getTotal() != null) {
            builder.setTotal(Int64Value.of(offsetResults.getTotal()));
        }
        return builder.build();
    }

    public static Common.FlowDescription.Builder toProtoLaunchFlowInfo(LaunchFlowInfo flowInfo) {
        var flowDescription = Common.FlowDescription.newBuilder();
        var flowProcessId = toProtoFlowProcessId(flowInfo.getFlowId());

        flowDescription.setFlowProcessId(flowProcessId);
        var flowLaunchDescription = flowInfo.getFlowDescription();
        if (flowLaunchDescription != null) {
            flowDescription.setTitle(flowLaunchDescription.getTitle());
            if (flowLaunchDescription.getDescription() != null) {
                flowDescription.setDescription(flowLaunchDescription.getDescription());
            }
            flowDescription.setFlowType(flowLaunchDescription.getFlowType());
        }
        if (flowLaunchDescription == null ||
                flowLaunchDescription.getFlowType() != Common.FlowType.FT_ROLLBACK) { // For old flows
            flowInfo.getRollbackFlows().stream()
                    .map(flowId -> flowProcessId.toBuilder().setId(flowId).build())
                    .forEach(flowDescription::addRollbackFlows);
        }
        return flowDescription;
    }

    public static CheckOuterClass.LargeAutostart toProtoLargeAutostart(
            Path configPath,
            LargeAutostartConfig config) {
        return CheckOuterClass.LargeAutostart.newBuilder()
                .setPath(configPath.toString())
                .setTarget(config.getTarget())
                .addAllToolchains(config.getToolchains())
                .build();
    }

    public static CheckOuterClass.NativeBuild toProtoNativeBuild(
            Path configPath,
            NativeBuildConfig config) {
        return CheckOuterClass.NativeBuild.newBuilder()
                .setPath(configPath.toString())
                .setToolchain(config.getToolchain())
                .addAllTarget(config.getTargets())
                .build();
    }

    public static Common.FlowVarsUi toProtoFlowVarUi(FlowVarsUi flowVarsUi) {
        return Common.FlowVarsUi.newBuilder()
                .setSchema(flowVarsUi.getSchema().toString())
                .build();
    }

    public static Common.XivaSubscription toXivaSubscription(String service, String topic) {
        return Common.XivaSubscription.newBuilder()
                .setService(service)
                .setTopic(topic)
                .build();
    }

    public static Common.VirtualProcessType toVirtualProcessType(@Nullable VirtualType virtualType) {
        return virtualType == null
                ? Common.VirtualProcessType.VP_NONE
                :
                switch (virtualType) {
                    case VIRTUAL_LARGE_TEST -> Common.VirtualProcessType.VP_LARGE_TESTS;
                    case VIRTUAL_NATIVE_BUILD -> Common.VirtualProcessType.VP_NATIVE_BUILDS;
                };
    }

    public static Common.TestId toTestId(ActionConfigState.TestId testId) {
        return Common.TestId.newBuilder()
                .setSuiteId(testId.getSuiteId())
                .setToolchain(testId.getToolchain())
                .build();
    }

    @Value
    static class StageStatusAndColumns {
        Common.StageState.StageStatus status;
        Set<Integer> differentPositions = new HashSet<>();
    }

    @Value
    static class LaunchAndVersion {
        LaunchId launchId;
        ru.yandex.ci.core.launch.versioning.Version version;

        @Nullable
        static LaunchAndVersion of(@Nullable FlowLaunchEntityForVersion entity) {
            if (entity == null) {
                return null;
            } else {
                return new LaunchAndVersion(entity.getLaunchId(), entity.getVersion());
            }
        }
    }

    private static Map<String, Set<String>> computeChildrenMap(
            Collection<ru.yandex.ci.flow.engine.runtime.state.model.JobState> jobStates
    ) {
        Map<String, Set<String>> childrenMap = new HashMap<>();

        for (var jobState : jobStates) {
            for (UpstreamLink<String> upstream : jobState.getUpstreams()) {
                childrenMap.computeIfAbsent(upstream.getEntity(), s -> new HashSet<>())
                        .add(jobState.getJobId());
            }
        }

        return childrenMap;
    }

}
