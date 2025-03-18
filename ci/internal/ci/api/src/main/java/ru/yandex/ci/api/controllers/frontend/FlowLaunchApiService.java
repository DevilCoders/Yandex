package ru.yandex.ci.api.controllers.frontend;

import java.time.Instant;
import java.util.Optional;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;
import java.util.function.Supplier;

import javax.annotation.Nonnull;

import io.grpc.Status;
import io.grpc.StatusRuntimeException;
import io.grpc.stub.StreamObserver;
import io.reactivex.disposables.Disposable;
import lombok.RequiredArgsConstructor;
import lombok.Value;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.bolts.function.Function;
import ru.yandex.ci.api.internal.frontend.flow.FrontendFlowApi.LaunchState;
import ru.yandex.ci.api.proto.Common;
import ru.yandex.ci.common.grpc.GrpcUtils;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.a.model.PermissionScope;
import ru.yandex.ci.core.db.EntityNotFoundException;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchTableFilter;
import ru.yandex.ci.core.security.OwnerConfig;
import ru.yandex.ci.core.security.PermissionsService;
import ru.yandex.ci.engine.proto.ProtoMappers;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.definition.job.InterruptMethod;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.FullJobLaunchId;
import ru.yandex.ci.flow.engine.runtime.JobInterruptionService;
import ru.yandex.ci.flow.engine.runtime.events.DisableJobManualSwitchEvent;
import ru.yandex.ci.flow.engine.runtime.events.EnableJobManualSwitchEvent;
import ru.yandex.ci.flow.engine.runtime.events.ExecutorInterruptingEvent;
import ru.yandex.ci.flow.engine.runtime.events.ForceSuccessTriggerEvent;
import ru.yandex.ci.flow.engine.runtime.events.JobEvent;
import ru.yandex.ci.flow.engine.runtime.events.ToggleSchedulerConstraintModifyEvent;
import ru.yandex.ci.flow.engine.runtime.events.TriggerEvent;
import ru.yandex.ci.flow.engine.runtime.state.FlowLaunchTable;
import ru.yandex.ci.flow.engine.runtime.state.FlowStateService;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.JobState;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChangeType;
import ru.yandex.ci.flow.engine.runtime.state.revision.FlowStateRevisionService;
import ru.yandex.ci.util.OffsetResults;
import ru.yandex.lang.NonNullApi;

@NonNullApi
@RequiredArgsConstructor
@Slf4j
public class FlowLaunchApiService {

    @Nonnull
    private final FlowStateService flowStateService;
    @Nonnull
    private final FlowStateRevisionService flowStateRevisionService;
    @Nonnull
    private final JobInterruptionService jobInterruptionService;
    @Nonnull
    private final PermissionsService permissionsService;
    @Nonnull
    private final CiDb db;

    public <T> void poll(Common.FlowLaunchId flowLaunchId,
                         long version,
                         Function<Optional<LaunchState>, T> map,
                         StreamObserver<T> observer) {

        Disposable disposable = flowStateRevisionService.observe(flowLaunchId.getId())
                .filter(r -> r > version)
                .timeout(45, TimeUnit.SECONDS) // Make sure we don't exceed timeout of GRPC call in frontend
                .firstOrError()
                .subscribe(
                        newVersion -> onNewVersion(flowLaunchId, map, observer),
                        error -> onError(flowLaunchId, map, observer, error)
                );
        log.info("Observing flow launch {}: {}", flowLaunchId.getId(), disposable);
    }

    private <T> void onNewVersion(Common.FlowLaunchId flowLaunchId,
                                  Function<Optional<LaunchState>, T> map,
                                  StreamObserver<T> observer) {
        wrapCall(
                observer,
                () -> map.apply(Optional.of(getLaunchState(flowLaunchId)))
        );
    }

    private <T> void onError(Common.FlowLaunchId flowLaunchId,
                             Function<Optional<LaunchState>, T> map,
                             StreamObserver<T> observer,
                             Throwable e) {
        if (e instanceof TimeoutException) {
            log.info("Timeout exceeds during subscription on {}", flowLaunchId);
            wrapCall(
                    observer,
                    () -> map.apply(Optional.empty())
            );
            return;
        }
        log.error("Error during subscription on {}", flowLaunchId, e);
        observer.onError(
                Status.INTERNAL
                        .withCause(e)
                        .withDescription(e.getMessage()).asException()
        );
    }

    public LaunchState getLaunchState(Common.FlowLaunchId flowLaunchId) {
        FlowLaunchEntity flowLaunch = getFlowLaunch(flowLaunchId);
        return getLaunchState(flowLaunch);
    }

    public LaunchState getLaunchState(FlowLaunchId flowLaunchId) {
        return db.currentOrReadOnly(() -> getLaunchState(getFlowLaunch(flowLaunchId)));
    }

    private LaunchState getLaunchState(FlowLaunchEntity flowLaunch) {
        Launch launch = db.currentOrReadOnly(() -> db.launches().get(flowLaunch.getLaunchId()));
        Common.StagesState stagesState = buildProtoStagesState(flowLaunch);
        return ProtoMappers.toProtoLaunchState(flowLaunch, launch, stagesState);
    }

    private Common.StagesState buildProtoStagesState(FlowLaunchEntity flowLaunch) {
        return db.currentOrReadOnly(() ->
                ProtoMappers.toProtoStagesState(flowLaunch,
                        db.stageGroup()::findByIds,
                        db.flowLaunch()::findLaunchIds));
    }

    public LaunchState launchJob(Common.FlowLaunchJobId flowLaunchJobId, String user) {
        checkPermissions(flowLaunchJobId, user, PermissionScope.START_JOB);

        FlowLaunchEntity flowLaunch = checkAndGetFlowLaunchRecalculatingState(
                flowLaunchJobId.getFlowLaunchId(),
                new TriggerEvent(flowLaunchJobId.getJobId(), user, false)
        );
        return getLaunchState(flowLaunch);
    }

    public ForceSuccessResult forceSuccess(Common.FlowLaunchJobId flowLaunchJobId, String user) {
        checkPermissions(flowLaunchJobId, user, PermissionScope.SKIP_JOB);

        var result = ForceSuccessResult.builder();
        var flowLaunch = getFlowLaunch(flowLaunchJobId.getFlowLaunchId());
        if (flowLaunch.isDisabled()) {
            log.info("Flow launch is disabled");
            return result
                    .launchState(getLaunchState(flowLaunch))
                    .message("Flow launch is disabled")
                    .build();
        }

        var job = flowLaunch.getJobs().get(flowLaunchJobId.getJobId());
        if (job == null) {
            throw new EntityNotFoundException("Job not found " + flowLaunchJobId.getJobId());
        }

        if (job.isDisabled()) {
            log.info("Job is disabled");
            return result
                    .launchState(getLaunchState(flowLaunch))
                    .message("Job is disabled")
                    .build();
        }

        flowLaunch = checkAndGetFlowLaunchRecalculatingState(
                flowLaunchJobId.getFlowLaunchId(),
                new ForceSuccessTriggerEvent(flowLaunchJobId.getJobId(), user)
        );

        return result
                .launchState(getLaunchState(flowLaunch))
                .build();
    }

    public LaunchState enableJobManualTrigger(Common.FlowLaunchJobId flowLaunchJobId, String user) {
        return executeJobManualTrigger(flowLaunchJobId, user, EnableJobManualSwitchEvent::new);
    }

    public LaunchState disableJobManualTrigger(Common.FlowLaunchJobId flowLaunchJobId, String user) {
        return executeJobManualTrigger(flowLaunchJobId, user, DisableJobManualSwitchEvent::new);
    }

    public LaunchState executeJobManualTrigger(
            Common.FlowLaunchJobId flowLaunchJobId,
            String user,
            JobManualTriggerEventSource eventSource) {
        checkPermissions(flowLaunchJobId, user, PermissionScope.MANUAL_TRIGGER);

        FlowLaunchEntity flowLaunch = getFlowLaunch(flowLaunchJobId.getFlowLaunchId());
        String jobId = flowLaunchJobId.getJobId();
        JobState job = flowLaunch.getJobs().get(jobId);
        if (job == null) {
            throw GrpcUtils.notFoundException("job " + jobId + " not found");
        }

        if (job.getLastLaunch() != null && !job.isManualTrigger()) {
            if (job.getLastLaunch().getLastStatusChangeType() == StatusChangeType.WAITING_FOR_STAGE) {
                throw GrpcUtils.failedPreconditionException("Job is already running and waiting for stage.");
            }
            if (job.isWaitingForScheduleChangeType()) {
                throw GrpcUtils.failedPreconditionException("Job is already running and waiting for schedule.");
            }
        }

        var event = eventSource.createEvent(jobId, user, Instant.now());

        FlowLaunchEntity recalculatedFlowLaunch = checkAndGetFlowLaunchRecalculatingState(
                flowLaunchJobId.getFlowLaunchId(), event
        );
        return getLaunchState(recalculatedFlowLaunch);
    }

    public LaunchState toggleSchedulerConstraints(Common.FlowLaunchJobId flowLaunchJobId, String user) {
        // TODO: remove
        checkPermissions(flowLaunchJobId, user, PermissionScope.MANUAL_TRIGGER);

        FlowLaunchEntity flowLaunch = getFlowLaunch(flowLaunchJobId.getFlowLaunchId());
        String jobId = flowLaunchJobId.getJobId();
        JobState job = flowLaunch.getJobs().get(jobId);
        if (job == null) {
            throw GrpcUtils.notFoundException("job " + jobId + " not found");
        }

        if (job.getLastLaunch() != null && job.isWaitingForScheduleChangeType()) {
            throw GrpcUtils.failedPreconditionException("Job is already running and waiting for schedule.");
        }

        ToggleSchedulerConstraintModifyEvent event =
                new ToggleSchedulerConstraintModifyEvent(jobId, user, Instant.now());

        FlowLaunchEntity recalculatedFlowLaunch = checkAndGetFlowLaunchRecalculatingState(
                flowLaunchJobId.getFlowLaunchId(), event
        );
        return getLaunchState(recalculatedFlowLaunch);
    }

    public LaunchState interrupt(Common.FlowLaunchJobId flowLaunchJobId, String user) {
        return interruptJob(flowLaunchJobId, user, false);
    }

    public LaunchState kill(Common.FlowLaunchJobId flowLaunchJobId, String user) {
        return interruptJob(flowLaunchJobId, user, true);
    }

    public OffsetResults<Launch> getFlowLaunchesByProcessId(
            CiProcessId processId,
            LaunchTableFilter filter,
            int offsetLaunchNumber,
            int limit
    ) {
        return OffsetResults.builder()
                .withTransactionReadOnly(db)
                .withItems(
                        limit,
                        (pLimit) -> db.launches().getLaunches(processId, filter, offsetLaunchNumber, pLimit)
                )
                // TODO: .withTotal(...) ?
                .fetch();
    }

    public FlowLaunchTable.FlowLaunchView toLaunchView(FlowLaunchId flowLaunchId) {
        return db.currentOrReadOnly(() -> db.flowLaunch().getLaunchView(flowLaunchId));
    }

    //


    private FlowLaunchEntity getFlowLaunch(Common.FlowLaunchId flowLaunchId) {
        return getFlowLaunch(ProtoMappers.toFlowLaunchId(flowLaunchId));
    }

    private FlowLaunchEntity getFlowLaunch(FlowLaunchId flowLaunchId) {
        return db.currentOrReadOnly(() -> db.flowLaunch().get(flowLaunchId));
    }

    private FlowLaunchEntity checkAndGetFlowLaunchRecalculatingState(Common.FlowLaunchId launchId, JobEvent jobEvent) {
        return flowStateService.recalc(ProtoMappers.toFlowLaunchId(launchId), jobEvent);
    }

    private LaunchState interruptJob(Common.FlowLaunchJobId flowLaunchJobId, String user, boolean kill) {
        checkPermissions(flowLaunchJobId, user, PermissionScope.KILL_JOB);

        String jobId = flowLaunchJobId.getJobId();
        FlowLaunchId flowLaunchId = ProtoMappers.toFlowLaunchId(flowLaunchJobId.getFlowLaunchId());
        FlowLaunchEntity flowLaunch = getFlowLaunch(flowLaunchJobId.getFlowLaunchId());

        JobState jobState = flowLaunch.getJobs().get(jobId);
        if (jobState == null) {
            throw GrpcUtils.notFoundException("job " + jobId + " not found");
        }

        flowLaunch = flowStateService.recalc(
                flowLaunchId,
                new ExecutorInterruptingEvent(
                        jobId,
                        jobState.getLastLaunch().getNumber(),
                        user
                )
        );

        jobInterruptionService.notifyExecutor(
                new FullJobLaunchId(
                        flowLaunchId,
                        jobId,
                        flowLaunch.getJobState(jobId).getLastLaunch().getNumber()
                ),
                kill ? InterruptMethod.KILL : InterruptMethod.INTERRUPT
        );

        return getLaunchState(flowLaunch);
    }

    private void checkPermissions(Common.FlowLaunchJobId flowLaunchJobId, String user, PermissionScope scope) {
        db.currentOrReadOnly(() -> {
            var cfg = this.checkPermissionsImpl(flowLaunchJobId.getFlowLaunchId(), user, scope);
            if (scope == PermissionScope.MANUAL_TRIGGER || scope == PermissionScope.START_JOB) {
                var jobId = flowLaunchJobId.getJobId();
                var flowLaunch = getFlowLaunch(flowLaunchJobId.getFlowLaunchId());
                var jobState = flowLaunch.getJobState(jobId);
                if (scope == PermissionScope.MANUAL_TRIGGER || jobState.awaitsManualTrigger()) {
                    var flowId = flowLaunch.getFlowInfo().getFlowId().getId();
                    permissionsService.checkJobApprovers(
                            user,
                            cfg.getCiProcessId(),
                            cfg.getConfigRevision(),
                            flowId,
                            jobId,
                            "manual trigger"
                    );
                }
            }
        });
    }

    private LaunchDetails checkPermissionsImpl(Common.FlowLaunchId flowLaunchId, String user, PermissionScope scope) {
        var view = db.flowLaunch().getLaunchAccessView(ProtoMappers.toFlowLaunchId(flowLaunchId));
        var ciProcessId = view.toCiProcessId();
        var configRevision = view.getFlowInfo().getConfigRevision();

        var cfg = OwnerConfig.of(view.getVcsInfo().getRevision(), view.getTriggeredBy());
        permissionsService.checkAccess(user, cfg, ciProcessId, configRevision, scope);
        log.info("Access granted for user = {}", user);

        return LaunchDetails.of(ciProcessId, configRevision);
    }

    private static <T> void wrapCall(StreamObserver<T> observer, Supplier<T> supplier) {
        try {
            T message = supplier.get();
            observer.onNext(message);
            observer.onCompleted();
        } catch (StatusRuntimeException e) {
            log.error("Error when processing request", e);
            observer.onError(e);
        } catch (Exception e) {
            log.error("Error when processing request", e);
            String description = e.getClass().getSimpleName();
            if (e.getMessage() != null) {
                description += ": " + e.getMessage();
            }
            observer.onError(Status.INTERNAL
                    .withDescription(description)
                    .withCause(e)
                    .asException()
            );
        }
    }

    interface JobManualTriggerEventSource {
        JobEvent createEvent(String jobId, String modifiedBy, Instant timestamp);
    }

    @Value(staticConstructor = "of")
    private static class LaunchDetails {
        CiProcessId ciProcessId;
        OrderedArcRevision configRevision;
    }
}
