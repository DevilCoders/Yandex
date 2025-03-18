package ru.yandex.ci.api.controllers.frontend;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.Optional;
import java.util.UUID;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import io.grpc.Status;
import io.grpc.StatusRuntimeException;
import lombok.RequiredArgsConstructor;

import ru.yandex.ci.api.internal.frontend.flow.FrontendFlowApi;
import ru.yandex.ci.api.internal.frontend.flow.FrontendFlowApi.JobLaunchPageModel;
import ru.yandex.ci.api.proto.Common;
import ru.yandex.ci.common.grpc.GrpcUtils;
import ru.yandex.ci.common.temporal.TemporalService;
import ru.yandex.ci.core.internal.InternalExecutorContext;
import ru.yandex.ci.engine.proto.ProtoMappers;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.TmsTaskId;
import ru.yandex.ci.flow.engine.runtime.di.ResourceService;
import ru.yandex.ci.flow.engine.runtime.di.model.AbstractResourceContainer;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.JobLaunch;
import ru.yandex.ci.flow.engine.runtime.state.model.JobState;
import ru.yandex.commune.bazinga.impl.FullJobId;
import ru.yandex.commune.bazinga.impl.OnetimeJob;
import ru.yandex.commune.bazinga.impl.storage.BazingaStorage;

@RequiredArgsConstructor
public class JobLaunchService {
    @Nonnull
    private final BazingaStorage bazingaStorage;
    @Nonnull
    private final ResourceService resourceService;
    @Nonnull
    private final CiDb db;
    @Nonnull
    private final TemporalService temporalService;

    public JobLaunchPageModel getJobLaunch(Common.JobLaunchId jobLaunchId) {
        FlowLaunchId flowLaunchId = ProtoMappers.toFlowLaunchId(jobLaunchId.getJobId().getFlowLaunchId());
        int launchNumber = jobLaunchId.getLaunchNumber();
        FlowLaunchEntity flowLaunch = db.currentOrReadOnly(() -> db.flowLaunch().findOptional(flowLaunchId))
                .orElseThrow(() -> new StatusRuntimeException(Status.NOT_FOUND));

        JobState jobState = flowLaunch.getJobState(jobLaunchId.getJobId().getJobId());

        JobLaunch launch = jobState.getLaunches().stream()
                .filter(l -> l.getNumber() == launchNumber)
                .findFirst()
                .orElseThrow(() ->
                        GrpcUtils.notFoundException(
                                "Unable to find a launch of job " + jobState.getJobId() + " with" +
                                        " number " + launchNumber +
                                        ", flowLaunchId = " + flowLaunchId
                        )
                );

        List<FrontendFlowApi.JobLaunchListItem> jobLaunchListItems = jobState.getLaunches().stream()
                .map(ProtoMappers::toProtoJobLaunchListItem)
                .sorted(Comparator.comparingInt(FrontendFlowApi.JobLaunchListItem::getNumber).reversed())
                .collect(Collectors.toList());

        List<String> workerHosts = new ArrayList<>();
        FullJobId bazingaFullJobId = null;
        TmsTaskId tmsTaskId = launch.getTmsTaskId();

        if (tmsTaskId != null && tmsTaskId.isBazinga()) {
            bazingaFullJobId = tmsTaskId.getBazingaId();
        }
        if (bazingaFullJobId != null) {
            OnetimeJob onetimeJob = bazingaStorage.findOnetimeJob(bazingaFullJobId).getOrNull();
            if (onetimeJob == null) {
                throw GrpcUtils.notFoundException("Unable to fetch onetime job with id " + bazingaFullJobId);
            }
            workerHosts = new ArrayList<>(onetimeJob.getWorkers());
        }

        AbstractResourceContainer consumed = resourceService.loadResources(launch.getConsumedResources());

        var producedRef = jobState.getDelegatedOutputResources() != null
                ? jobState.getDelegatedOutputResources().getResourceRef()
                : launch.getProducedResources();
        AbstractResourceContainer produced = resourceService.loadResources(producedRef);

        JobLaunchPageModel.Builder builder = JobLaunchPageModel.newBuilder();
        fillTemporalInfo(builder, tmsTaskId);

        launch.getStatusHistory().stream()
                .map(ProtoMappers::toProtoStatusChangeType)
                .forEach(builder::addStatusHistory);
        launch.getTaskStates().stream()
                .map(ProtoMappers::toProtoTaskState)
                .forEach(builder::addTaskStates);

        Optional.ofNullable(bazingaFullJobId)
                .map(FullJobId::toSerializedString)
                .ifPresent(builder::setBazingaFullJobId);

        Optional.ofNullable(jobState.getTitle())
                .ifPresent(builder::setJobTitle);
        Optional.ofNullable(jobState.getDescription())
                .ifPresent(builder::setJobDescription);
        Optional.ofNullable(launch.getTriggeredBy())
                .ifPresent(builder::setTriggeredBy);
        Optional.ofNullable(launch.getInterruptedBy())
                .ifPresent(builder::setInterruptedBy);
        Optional.ofNullable(launch.getForceSuccessTriggeredBy())
                .ifPresent(value -> {
                    builder.setForceSuccessTriggeredBy(value);
                    builder.setIsForceSuccess(true);
                });
        Optional.ofNullable(launch.getSubscriberExceptionStacktrace())
                .ifPresent(builder::setSubscriberExceptionStackTrace);
        Optional.ofNullable(launch.getExecutionExceptionStacktrace())
                .ifPresent(builder::setExecutionExceptionStackTrace);


        var sourceCodeId = Optional.ofNullable(jobState.getExecutorContext().getInternal())
                .map(InternalExecutorContext::getExecutor)
                .map(UUID::toString)
                .orElse("");

        var lastType = launch.getLastStatusChangeType();
        builder.setFlowLaunchId(flowLaunchId.asString())
                .setIsFlowLaunchDisabled(flowLaunch.isDisabled())
                .setIsFlowLaunchDisablingGracefully(flowLaunch.isDisablingGracefully())
                .setIsShouldIgnoreUninterruptableStages(flowLaunch.shouldIgnoreUninterruptibleStages())
                .setJobSourceCodeId(sourceCodeId)
                .setJobId(jobState.getJobId())
                .setNumber(launch.getNumber())
                .setStatus(ProtoMappers.toProtoStatusChangeType(lastType))
                .setStatus2(ProtoMappers.toProtoStatusChangeType2(lastType))
                .setLaunchDate(ProtoMappers.toProtoTimestamp(launch.getStatusHistory().get(0).getDate()))
                .addAllWorkerHosts(workerHosts)
                .addAllLaunchList(jobLaunchListItems)
                .setIsReadyToRun(jobState.isReadyToRun())
                .addAllConsumedResources(ProtoMappers.toProtoJobLaunchResourceViewModels(consumed))
                .addAllProducedResources(ProtoMappers.toProtoJobLaunchResourceViewModels(produced));

        return builder.build();
    }

    private void fillTemporalInfo(JobLaunchPageModel.Builder builder, @Nullable TmsTaskId tmsTaskId) {
        if (tmsTaskId == null) {
            return;
        }
        if (!tmsTaskId.isTemporal()) {
            return;
        }

        var info = FrontendFlowApi.TemporalWorkflowInfo.newBuilder()
                .setWorkflowId(tmsTaskId.getTemporalWorkflowId())
                .setWorkflowUrl(temporalService.getWorkflowUrl(tmsTaskId.getTemporalWorkflowId()));

        builder.setTemporalWorkflowInfo(info);
    }

}
