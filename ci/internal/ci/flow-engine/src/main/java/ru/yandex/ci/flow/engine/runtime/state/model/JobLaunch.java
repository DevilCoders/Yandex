package ru.yandex.ci.flow.engine.runtime.state.model;

import java.time.Instant;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Optional;
import java.util.function.Function;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import lombok.AllArgsConstructor;
import lombok.Data;

import ru.yandex.ci.client.sandbox.api.SandboxTaskStatus;
import ru.yandex.ci.flow.engine.definition.context.impl.SupportType;
import ru.yandex.ci.flow.engine.runtime.TmsTaskId;
import ru.yandex.ci.flow.engine.runtime.di.model.ResourceRefContainer;
import ru.yandex.ci.ydb.Persisted;
import ru.yandex.tasklet.api.v2.DataModel;

@Persisted
@Data
@AllArgsConstructor
public class JobLaunch {
    private int number;
    @Nullable
    private String triggeredBy;
    private String forceSuccessTriggeredBy;
    private String interruptedBy;

    private List<UpstreamLaunch> upstreamLaunches;
    private List<StatusChange> statusHistory;

    private ResourceRefContainer producedResources;
    private ResourceRefContainer consumedResources;

    @Nullable
    private SandboxTaskStatus sandboxTaskStatus;
    @Nullable
    private DataModel.ErrorCodes.ErrorCode taskletServerError;
    private String executionExceptionStacktrace;
    private String subscriberExceptionStacktrace;
    private List<SupportType> supportInfo;

    @Deprecated
    @Nullable
    private String bazingaFullJobIdString;

    @Nullable
    private TmsTaskId tmsTaskId;

    private Float totalProgress;
    private String statusText;

    @Nullable
    private LinkedHashMap<String, TaskBadge> taskStates;

    @Nullable
    private Instant scheduleTime;

    public JobLaunch(
            int number,
            @Nullable String triggeredBy,
            List<UpstreamLaunch> upstreamLaunches,
            List<StatusChange> statusHistory
    ) {
        this.number = number;
        this.triggeredBy = triggeredBy;
        this.upstreamLaunches = upstreamLaunches;
        this.statusHistory = statusHistory;
        this.consumedResources = ResourceRefContainer.empty();
        this.producedResources = ResourceRefContainer.empty();
    }

    public StatusChange getLastStatusChange() {
        return statusHistory.get(statusHistory.size() - 1);
    }

    public boolean isLastStatusChangeTypeFailed() {
        return getLastStatusChangeType().isFailed();
    }

    public StatusChange getFirstStatusChange() {
        return statusHistory.get(0);
    }

    public StatusChangeType getLastStatusChangeType() {
        return getLastStatusChange().getType();
    }

    public void recordStatusChange(StatusChange statusChange) {
        statusHistory.add(statusChange);
    }

    public void setTmsTaskId(TmsTaskId tmsTaskId) {
        this.tmsTaskId = tmsTaskId;
    }

    @Nullable
    public TmsTaskId getTmsTaskId() {
        if (tmsTaskId == null && bazingaFullJobIdString != null) {
            tmsTaskId = TmsTaskId.fromBazingaId(bazingaFullJobIdString);
        }
        return tmsTaskId;
    }

    public List<TaskBadge> getTaskStates() {
        return taskStates == null ? List.of() : List.copyOf(taskStates.values());
    }

    public Optional<TaskBadge> getTaskState(String id) {
        return taskStates == null ? Optional.empty() : Optional.ofNullable(taskStates.get(id));
    }

    public void setTaskStates(List<TaskBadge> taskStates) {
        this.taskStates = taskStates.stream()
                .collect(
                        Collectors.toMap(
                                TaskBadge::getId,
                                Function.identity(),
                                (a, b) -> {
                                    throw new RuntimeException("duplicate ids " + a + " and " + b);
                                },
                                LinkedHashMap::new
                        )
                );
    }

    public void setTaskState(TaskBadge taskBadge) {
        Preconditions.checkNotNull(taskBadge);
        if (this.taskStates == null) {
            this.taskStates = new LinkedHashMap<>();
        }
        this.taskStates.put(taskBadge.getId(), taskBadge);
    }
}
