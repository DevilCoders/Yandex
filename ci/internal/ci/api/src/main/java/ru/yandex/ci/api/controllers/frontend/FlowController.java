package ru.yandex.ci.api.controllers.frontend;

import java.net.UnknownHostException;
import java.util.Objects;
import java.util.Optional;

import javax.annotation.Nonnull;

import io.grpc.stub.StreamObserver;
import joptsimple.internal.Strings;
import lombok.AllArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.client.ResourceAccessException;
import org.springframework.web.client.RestTemplate;

import ru.yandex.ci.api.internal.frontend.flow.FlowServiceGrpc;
import ru.yandex.ci.api.internal.frontend.flow.FrontendFlowApi;
import ru.yandex.ci.api.internal.frontend.flow.FrontendFlowApi.BazingaLogRequest;
import ru.yandex.ci.api.internal.frontend.flow.FrontendFlowApi.BazingaLogResponse;
import ru.yandex.ci.api.internal.frontend.flow.FrontendFlowApi.DisableJobManualTriggerRequest;
import ru.yandex.ci.api.internal.frontend.flow.FrontendFlowApi.DisableJobManualTriggerResponse;
import ru.yandex.ci.api.internal.frontend.flow.FrontendFlowApi.EnableJobManualTriggerRequest;
import ru.yandex.ci.api.internal.frontend.flow.FrontendFlowApi.EnableJobManualTriggerResponse;
import ru.yandex.ci.api.internal.frontend.flow.FrontendFlowApi.FetchJobLaunchDetailsRequest;
import ru.yandex.ci.api.internal.frontend.flow.FrontendFlowApi.FetchJobLaunchDetailsResponse;
import ru.yandex.ci.api.internal.frontend.flow.FrontendFlowApi.ForceSuccessRequest;
import ru.yandex.ci.api.internal.frontend.flow.FrontendFlowApi.ForceSuccessResponse;
import ru.yandex.ci.api.internal.frontend.flow.FrontendFlowApi.InterruptJobRequest;
import ru.yandex.ci.api.internal.frontend.flow.FrontendFlowApi.InterruptJobResponse;
import ru.yandex.ci.api.internal.frontend.flow.FrontendFlowApi.KillJobRequest;
import ru.yandex.ci.api.internal.frontend.flow.FrontendFlowApi.KillJobResponse;
import ru.yandex.ci.api.internal.frontend.flow.FrontendFlowApi.LaunchJobRequest;
import ru.yandex.ci.api.internal.frontend.flow.FrontendFlowApi.LaunchJobResponse;
import ru.yandex.ci.api.internal.frontend.flow.FrontendFlowApi.LaunchState;
import ru.yandex.ci.api.internal.frontend.flow.FrontendFlowApi.LoadLaunchDetailsRequest;
import ru.yandex.ci.api.internal.frontend.flow.FrontendFlowApi.LoadLaunchDetailsResponse;
import ru.yandex.ci.api.internal.frontend.flow.FrontendFlowApi.PollLaunchStateRequest;
import ru.yandex.ci.api.internal.frontend.flow.FrontendFlowApi.PollLaunchStateResponse;
import ru.yandex.ci.api.internal.frontend.flow.FrontendFlowApi.ToggleJobSchedulerConstraintRequest;
import ru.yandex.ci.api.internal.frontend.flow.FrontendFlowApi.ToggleJobSchedulerConstraintResponse;
import ru.yandex.ci.api.misc.AuthUtils;
import ru.yandex.ci.common.bazinga.S3LogStorage;
import ru.yandex.ci.common.grpc.GrpcUtils;
import ru.yandex.ci.common.temporal.logging.TemporalLogService;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.engine.proto.ProtoMappers;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.commune.bazinga.impl.FullJobId;

@AllArgsConstructor
@Slf4j
public class FlowController extends FlowServiceGrpc.FlowServiceImplBase {

    @Nonnull
    private final FlowLaunchApiService flowLaunchApiService;
    @Nonnull
    private final JobLaunchService jobLaunchService;
    @Nonnull
    private final S3LogStorage s3LogStorage;
    @Nonnull
    private final TemporalLogService temporalLogService;

    private final int tmsHttpPort;

    @Override
    public void pollLaunchState(PollLaunchStateRequest request,
                                StreamObserver<PollLaunchStateResponse> responseObserver) {

        flowLaunchApiService.poll(
                request.getFlowLaunchId(),
                request.getStateVersion(),
                launchStateOptional -> {
                    if (launchStateOptional.isEmpty()) {
                        return PollLaunchStateResponse.newBuilder()
                                .setUpdated(false)
                                .build();
                    }
                    return PollLaunchStateResponse.newBuilder()
                            .setUpdated(true)
                            .setLaunchState(launchStateOptional.get()).build();

                },
                responseObserver
        );
    }

    @Override
    public void launchJob(LaunchJobRequest request,
                          StreamObserver<LaunchJobResponse> responseObserver) {
        LaunchState launchState = flowLaunchApiService.launchJob(request.getFlowLaunchJobId(), getUsername());
        var response = LaunchJobResponse.newBuilder()
                .setLaunchState(launchState)
                .build();
        responseObserver.onNext(response);
        responseObserver.onCompleted();
    }

    @Override
    public void interruptJob(InterruptJobRequest request,
                             StreamObserver<InterruptJobResponse> responseObserver) {
        LaunchState launchState = flowLaunchApiService.interrupt(
                request.getFlowLaunchJobId(),
                getUsername()
        );

        var response = InterruptJobResponse.newBuilder()
                .setLaunchState(launchState)
                .build();

        responseObserver.onNext(response);
        responseObserver.onCompleted();
    }

    @Override
    public void killJob(KillJobRequest request,
                        StreamObserver<KillJobResponse> responseObserver) {

        LaunchState launchState = flowLaunchApiService.kill(
                request.getFlowLaunchJobId(),
                getUsername()
        );

        var response = KillJobResponse.newBuilder()
                .setLaunchState(launchState)
                .build();

        responseObserver.onNext(response);
        responseObserver.onCompleted();
    }

    @Override
    public void enableJobManualTrigger(EnableJobManualTriggerRequest request,
                                       StreamObserver<EnableJobManualTriggerResponse> responseObserver) {
        var launchState = flowLaunchApiService.enableJobManualTrigger(
                request.getFlowLaunchJobId(),
                getUsername()
        );

        var response = EnableJobManualTriggerResponse.newBuilder()
                .setLaunchState(launchState)
                .build();

        responseObserver.onNext(response);
        responseObserver.onCompleted();
    }

    @Override
    public void disableJobManualTrigger(DisableJobManualTriggerRequest request,
                                        StreamObserver<DisableJobManualTriggerResponse> responseObserver) {
        var launchState = flowLaunchApiService.disableJobManualTrigger(
                request.getFlowLaunchJobId(),
                getUsername()
        );

        var response = DisableJobManualTriggerResponse.newBuilder()
                .setLaunchState(launchState)
                .build();

        responseObserver.onNext(response);
        responseObserver.onCompleted();
    }

    @Override
    public void toggleJobSchedulerConstraint(ToggleJobSchedulerConstraintRequest request,
                                             StreamObserver<ToggleJobSchedulerConstraintResponse> responseObserver) {
        LaunchState launchState = flowLaunchApiService.toggleSchedulerConstraints(
                request.getFlowLaunchJobId(),
                getUsername()
        );

        var response = ToggleJobSchedulerConstraintResponse.newBuilder()
                .setLaunchState(launchState)
                .build();
        responseObserver.onNext(response);
        responseObserver.onCompleted();
    }

    @Override
    public void forceSuccess(ForceSuccessRequest request,
                             StreamObserver<ForceSuccessResponse> responseObserver) {
        var result = flowLaunchApiService.forceSuccess(request.getFlowLaunchJobId(), getUsername());
        var response = ForceSuccessResponse.newBuilder()
                .setLaunchState(result.getLaunchState());
        Optional.ofNullable(result.getMessage())
                .ifPresent(response::setErrorMessage);

        responseObserver.onNext(response.build());
        responseObserver.onCompleted();
    }

    @Override
    public void fetchJobLaunchDetails(FetchJobLaunchDetailsRequest request,
                                      StreamObserver<FetchJobLaunchDetailsResponse> responseObserver) {

        FrontendFlowApi.JobLaunchPageModel jobLaunchPageModel =
                jobLaunchService.getJobLaunch(request.getJobLaunchId());

        var response = FetchJobLaunchDetailsResponse.newBuilder()
                .setJobLaunchPageModel(jobLaunchPageModel)
                .build();
        responseObserver.onNext(response);
        responseObserver.onCompleted();
    }

    @Override
    public void loadLaunchDetails(LoadLaunchDetailsRequest request,
                                  StreamObserver<LoadLaunchDetailsResponse> responseObserver) {
        FlowLaunchId flowLaunchId;
        if (request.getFlowLaunchId().getId().isEmpty()) {
            var processId = ProtoMappers.toCiProcessId(request.getFlowProcessId());
            flowLaunchId = FlowLaunchId.of(LaunchId.of(processId, request.getNumber()));
        } else {
            flowLaunchId = ProtoMappers.toFlowLaunchId(request.getFlowLaunchId());
        }

        var launchState = flowLaunchApiService.getLaunchState(flowLaunchId);

        var response = LoadLaunchDetailsResponse.newBuilder()
                .setLaunchState(launchState)
                .build();
        responseObserver.onNext(response);
        responseObserver.onCompleted();
    }

    @Override
    public void bazingaLog(BazingaLogRequest request,
                           StreamObserver<BazingaLogResponse> responseObserver) {
        log.info("Getting logs from host {}, full job id {}...", request.getBazingaHost(), request.getFullJobId());

        if (Strings.isNullOrEmpty(request.getBazingaHost()) || Strings.isNullOrEmpty(request.getFullJobId())) {
            log.warn("Uncorrect request: {}", request);
            throw GrpcUtils.invalidArgumentException("Request host or job id is null or empty");
        }

        try {
            tryLoadBazingaLogsFromHost(request, responseObserver);
            return;
        } catch (ResourceAccessException e) {
            log.error("Unable to load logs: {}", request, e);
            if (!(e.getCause() instanceof UnknownHostException)) {
                throw e;
            }
        }

        var parsedJobId = FullJobId.parse(request.getFullJobId());
        var stream = s3LogStorage.getInputStreamS3(request.getBazingaHost(), parsedJobId);
        if (stream.isPresent()) {
            var response = BazingaLogResponse.newBuilder()
                    .setLog(stream.get().readText())
                    .build();
            responseObserver.onNext(response);
            responseObserver.onCompleted();
        } else {
            throw GrpcUtils.notFoundException("No logs provided or already expired");
        }
    }

    @Override
    public void temporalLog(FrontendFlowApi.TemporalLogRequest request,
                            StreamObserver<FrontendFlowApi.TemporalLogResponse> responseObserver) {
        String workflowId = request.getWorkflowId();
        if (Strings.isNullOrEmpty(workflowId)) {
            throw GrpcUtils.invalidArgumentException("workflow_id must be provided");
        }

        log.info("Getting logs for workflow {}", workflowId);
        String log = temporalLogService.getLog(workflowId);
        if (log == null) {
            throw GrpcUtils.notFoundException("No logs found for workflow: " + workflowId);
        }
        responseObserver.onNext(FrontendFlowApi.TemporalLogResponse.newBuilder().setLog(log).build());
        responseObserver.onCompleted();
    }

    @Override
    public void toLaunchId(
            FrontendFlowApi.ToLaunchIdRequest request,
            StreamObserver<FrontendFlowApi.ToLaunchIdResponse> responseObserver
    ) {
        var launchView = flowLaunchApiService.toLaunchView(ProtoMappers.toFlowLaunchId(request.getFlowLaunchId()));
        responseObserver.onNext(FrontendFlowApi.ToLaunchIdResponse.newBuilder()
                .setLaunchId(ProtoMappers.toProtoLaunchId(launchView.toLaunchId()))
                .setProjectId(launchView.getProjectId())
                .build());
        responseObserver.onCompleted();
    }

    private void tryLoadBazingaLogsFromHost(BazingaLogRequest request,
                                            StreamObserver<BazingaLogResponse> responseObserver) {
        // Проще оставить как есть, пока не решим проблему с логами в CI-1691
        RestTemplate template = new RestTemplate();
        String urlForLogs = "http://" + request.getBazingaHost() + ":" + tmsHttpPort
                + "/z/bazinga/log/" + request.getFullJobId();

        ResponseEntity<String> logResponse = template.getForEntity(urlForLogs, String.class);

        if (HttpStatus.NOT_FOUND.equals(logResponse.getStatusCode())) {
            log.warn(logResponse.toString());
            throw GrpcUtils.notFoundException("No logs provided or already expired");
        } else if (!HttpStatus.OK.equals(logResponse.getStatusCode())) {
            log.error(logResponse.toString());
            throw GrpcUtils.internalError(logResponse.toString());
        }

        var response = BazingaLogResponse.newBuilder()
                .setLog(Objects.requireNonNullElse(logResponse.getBody(), "No logs provided"))
                .build();
        responseObserver.onNext(response);
        responseObserver.onCompleted();
    }

    private static String getUsername() {
        return AuthUtils.getUsername();
    }
}
