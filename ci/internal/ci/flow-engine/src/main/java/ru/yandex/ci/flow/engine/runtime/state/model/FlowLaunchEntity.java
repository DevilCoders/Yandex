package ru.yandex.ci.flow.engine.runtime.state.model;

import java.time.Instant;
import java.util.Collection;
import java.util.List;
import java.util.Map;
import java.util.NoSuchElementException;
import java.util.Optional;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import com.google.common.base.Strings;
import com.google.common.collect.Iterables;
import lombok.AccessLevel;
import lombok.Builder;
import lombok.Getter;
import lombok.Singular;
import lombok.Value;
import lombok.With;
import one.util.streamex.StreamEx;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.FlowFullId;
import ru.yandex.ci.core.launch.LaunchFlowInfo;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.launch.LaunchVcsInfo;
import ru.yandex.ci.flow.engine.definition.stage.Stage;
import ru.yandex.ci.flow.engine.definition.stage.StageRef;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.di.model.ResourceRefContainer;

@SuppressWarnings("ReferenceEquality")
@Value
@Builder(toBuilder = true, buildMethodName = "buildInternal")
@Table(name = "flow/FlowLaunch")
public class FlowLaunchEntity implements FlowLaunchRef, Entity<FlowLaunchEntity> {
    /*
    Additional sql:
    ALTER TABLE `flow/FlowLaunch` SET (AUTO_PARTITIONING_MIN_PARTITIONS_COUNT=10);
    ALTER TABLE `flow/FlowLaunch` SET (AUTO_PARTITIONING_BY_LOAD = ENABLED);
    */
    @Column
    @Nonnull
    FlowLaunchEntity.Id id;

    @With
    @Column(dbType = DbType.UTF8)
    @Nonnull
    String processId;

    @Column
    int launchNumber;

    //Abc service
    @Column(dbType = DbType.UTF8)
    @Nonnull
    String projectId;

    @Column(dbType = DbType.JSON, flatten = false)
    @With
    @Nullable
    GracefulDisablingState gracefulDisablingState;

    @Column(dbType = DbType.UTF8)
    @Nonnull
    @lombok.Builder.Default
    LaunchState state = LaunchState.RUNNING;

    @Column(dbType = DbType.JSON, flatten = false)
    @Nonnull
    @lombok.Builder.Default
    LaunchStatistics statistics = LaunchStatistics.EMPTY;

    @Column(dbType = DbType.TIMESTAMP)
    @Nonnull
    Instant createdDate;

    @Column(dbType = DbType.JSON, flatten = false)
    @Nonnull
    @lombok.Builder.Default
    ResourceRefContainer manualResources = ResourceRefContainer.empty();

    @Column(dbType = DbType.JSON, flatten = false)
    @Nonnull
    @Singular
    @With
    Map<String, JobState> jobs;

    @Column(dbType = DbType.JSON, flatten = false)
    @Nullable
    @Singular
    @With
    Map<String, JobState> cleanupJobs;

    @Column(dbType = DbType.JSON, flatten = false)
    @Nonnull
    @Singular
    List<StoredStage> stages;

    @Column
    @With
    boolean disabled;

    @Column
    @With(AccessLevel.PRIVATE)
    int stateVersion;

    @Column(dbType = DbType.JSON, flatten = false)
    @Nonnull
    LaunchVcsInfo vcsInfo;

    @Column(dbType = DbType.JSON, flatten = false)
    @Nonnull
    LaunchFlowInfo flowInfo;

    @Column(dbType = DbType.JSON, flatten = false)
    @Nonnull
    LaunchInfo launchInfo;

    @Column(dbType = DbType.UTF8)
    @Nullable
    String triggeredBy;

    @Column(dbType = DbType.UTF8)
    @Nullable
    String title;

    @Getter(AccessLevel.NONE)
    @Deprecated
    @Column(dbType = DbType.UTF8)
    String stageGroupId;

    @Getter(AccessLevel.NONE)
    @Deprecated
    @Column
    Boolean skipStagesAllowed;

    @Nonnull
    @Override
    public FlowLaunchEntity.Id getId() {
        return id;
    }

    @Override
    public FlowLaunchId getFlowLaunchId() {
        return FlowLaunchId.of(id.id);
    }

    @Override
    public FlowFullId getFlowFullId() {
        return flowInfo.getFlowId();
    }

    public LaunchId getLaunchId() {
        return LaunchId.of(CiProcessId.ofString(processId), launchNumber);
    }

    public boolean shouldIgnoreUninterruptibleStages() {
        return gracefulDisablingState != null && gracefulDisablingState.isIgnoreUninterruptibleStage();
    }

    public boolean isDisablingGracefully() {
        return gracefulDisablingState != null && gracefulDisablingState.isInProgress();
    }

    public Map<String, JobState> getCleanupJobs() {
        return cleanupJobs == null ? Map.of() : cleanupJobs;
    }

    public boolean hasCleanupJobs() {
        return cleanupJobs != null && !cleanupJobs.isEmpty();
    }

    public boolean isCleanupRunning() {
        return jobs.values().stream()
                .anyMatch(state -> state.getJobType() == JobType.CLEANUP);
    }

    public JobState getJobState(String jobId) {
        JobState jobState = jobs.get(jobId);
        Preconditions.checkNotNull(jobState, "Unable to find job state with id " + jobId);
        return jobState;
    }

    public FlowLaunchEntity withIncrementedVersion() {
        return withStateVersion(stateVersion + 1);
    }

    public List<JobState> getJobsByStage(StageRef stage) {
        return jobs.values().stream().filter(j -> stage.equals(j.getStage())).collect(Collectors.toList());
    }

    public List<JobState> getDownstreams(String jobId) {
        return jobs.values().stream()
                .filter(j ->
                        j.getUpstreams().stream().anyMatch(
                                u -> jobId.equals(u.getEntity())
                        )
                ).collect(Collectors.toList());
    }

    public List<JobState> getDownstreamsRecursive(String jobId) {
        return getDownstreams(jobId).stream()
                .flatMap(downstream -> Stream.concat(
                        Stream.of(downstream),
                        getDownstreamsRecursive(downstream.getJobId()).stream()
                ))
                .distinct()
                .collect(Collectors.toList());
    }

    public boolean hasRunningJobs() {
        return jobs.values().stream().anyMatch(JobState::isInProgress);
    }

    public boolean hasFailedJobs() {
        return jobs.values().stream().anyMatch(JobState::isFailed);
    }

    public List<JobState> getFailedJobs() {
        return jobs.values().stream().filter(JobState::isFailed).collect(Collectors.toList());
    }

    public boolean hasNeedsManualTriggerJobs() {
        return !disabled && !isDisablingGracefully() &&
                jobs.values().stream().anyMatch(JobState::awaitsManualTrigger);
    }

    public boolean hasNoRunningJobsOnStage(StageRef stage) {
        return getJobsByStage(stage).stream()
                .allMatch(j -> {
                    StatusChangeType lastStatusChangeType = j.getLastStatusChangeType();
                    return lastStatusChangeType == null || lastStatusChangeType.isFinished();
                });
    }

    public Optional<String> getStageGroupId() {
        return Optional.ofNullable(flowInfo.getStageGroupId());
    }

    public boolean isStaged() {
        return getStageGroupId().isPresent();
    }

    public boolean isSkipStagesAllowed() {
        return flowInfo.isSkipStagesAllowed();
    }

    public String getIdString() {
        return id.id;
    }

    public List<JobState> getJobsThatNeedManualTrigger() {
        return jobs.values().stream().filter(JobState::awaitsManualTrigger).collect(Collectors.toList());
    }

    @Nullable
    public OrderedArcRevision getPreviousRevision() {
        return vcsInfo.getPreviousRevision();
    }

    public OrderedArcRevision getTargetRevision() {
        return vcsInfo.getRevision();
    }

    public boolean hasStage(String id) {
        return this.stages.stream().anyMatch(stage -> stage.getId().equals(id));
    }

    public StoredStage getStage(@Nonnull StageRef stageRef) {
        return stages.stream().filter(s -> s.getId().equals(stageRef.getId()))
                .findFirst()
                .orElseThrow(() -> new NoSuchElementException(
                        String.format("Unable to find stage with id %s in group %s", stageRef.getId(), this.id))
                );
    }

    /**
     * Checks whether stage is less than all given stages. You must not use stages from different flow launches.
     *
     * @param stageId  stage to check
     * @param stageIds stages from this flow launch
     * @param matchAll false to skip all stages we have no information about
     * @return true if stage is less then all stages
     */
    public boolean isStageLessThanAll(String stageId, Collection<String> stageIds, boolean matchAll) {
        if (!matchAll && !hasStage(stageId)) {
            return true;
        }
        return stageIds.stream()
                .filter(id -> matchAll || hasStage(id))
                .allMatch(s -> isStageLessThan(stageId, s));
    }

    /**
     * Checks whether stage is greater than all given stages. You must not use stages from different flow launches.
     *
     * @param stageId  stage to check
     * @param stageIds stages from this flow launch
     * @param matchAll false to skip all stages we have no information about
     * @return true if stage is greater then all stages
     */
    public boolean isStageGreaterThanAll(String stageId, Collection<String> stageIds, boolean matchAll) {
        if (!matchAll && !hasStage(stageId)) {
            return true;
        }
        return stageIds.stream()
                .filter(id -> matchAll || hasStage(id))
                .allMatch(s -> isStageGreaterThan(stageId, s));
    }

    /**
     * Checks whether stage is less than any given stages. You must not use stages from different flow launches.
     *
     * @param stageId  stage to check
     * @param stageIds stages from this flow launch
     * @return true if stage is less then any stages
     */
    public boolean isStageLessThanAny(String stageId, Collection<String> stageIds) {
        return stageIds.stream().anyMatch(s -> isStageLessThan(stageId, s));
    }

    private int getStageIndex(String stageId, String side) {
        int index = Iterables.indexOf(stages, stage -> stage.getId().equals(stageId));
        Preconditions.checkArgument(index >= 0,
                "Unable to find %s stage with id %s", side, stageId);
        return index;
    }

    private boolean isStageLessThan(String leftStageId, String rightStageId) {
        return getStageIndex(leftStageId, "left") < getStageIndex(rightStageId, "right");
    }

    private boolean isStageGreaterThan(String leftStageId, String rightStageId) {
        return getStageIndex(leftStageId, "left") > getStageIndex(rightStageId, "right");
    }

    @Value(staticConstructor = "of")
    public static class Id implements Entity.Id<FlowLaunchEntity> {
        @Nonnull
        @Column(name = "id", dbType = DbType.UTF8)
        String id;
    }

    public static class Builder {

        public Builder id(FlowLaunchEntity.Id id) {
            this.id = id;
            return this;
        }

        public Builder id(String id) {
            return id(FlowLaunchEntity.Id.of(id));
        }

        public Builder id(@Nonnull FlowLaunchId id) {
            return id(id.asString());
        }

        public Builder launchId(@Nonnull LaunchId launchId) {
            this.processId = launchId.getProcessId().asString();
            this.launchNumber = launchId.getNumber();
            return this;
        }

        public FlowLaunchEntity build() {
            Preconditions.checkArgument(!Strings.isNullOrEmpty(projectId),
                    "projectId must be configured");

            Preconditions.checkArgument(flowInfo.getStageGroupId() != null || stages == null || stages.isEmpty(),
                    "FlowLaunchEntity must either have stages and flowInfo.getStageGroupId() or no stages at all");

            return buildInternal();
        }

        public Builder rawStages(List<Stage> stages) {
            return stages(StreamEx.of(stages).map(StoredStage::of).toList());
        }

    }


}
