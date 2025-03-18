package ru.yandex.ci.engine.autocheck.jobs.ci;

import java.time.Duration;
import java.time.temporal.ChronoUnit;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

import javax.annotation.Nullable;

import ci.tasklets.run_ci_action.RunCiAction;
import com.google.common.base.Preconditions;
import com.google.common.base.Strings;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.api.internal.frontend.flow.oncommit.FrontendOnCommitFlowLaunchApi;
import ru.yandex.ci.api.proto.Common;
import ru.yandex.ci.client.ci.CiClient;
import ru.yandex.ci.client.ci.CiClientImpl;
import ru.yandex.ci.client.tvm.grpc.OAuthCallCredentials;
import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.common.grpc.GrpcClientProperties;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.launch.LaunchRuntimeInfo;
import ru.yandex.ci.core.poller.Poller;
import ru.yandex.ci.core.time.DurationParser;
import ru.yandex.ci.engine.autocheck.jobs.BaseSecretsReceiverJob;
import ru.yandex.ci.engine.flow.SecurityAccessService;
import ru.yandex.ci.engine.proto.ProtoMappers;
import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.job.ExecutorInfo;
import ru.yandex.ci.flow.engine.definition.resources.Consume;
import ru.yandex.ci.flow.engine.runtime.state.model.TaskBadge;
import ru.yandex.ci.flow.utils.UrlService;

@Slf4j
@RequiredArgsConstructor
@ExecutorInfo(
        title = "Start CI Action and wait for its completion recheck",
        description = "Internal job for starting autocheck recheck task"
)
@Consume(name = "config", proto = RunCiAction.Config.class)
public class RunCiActionJob extends BaseSecretsReceiverJob {

    public static final String TASK_BADGE_ID = "custom-action";

    public static final UUID ID = UUID.fromString("77be55d8-1342-4f47-88a6-e0fc549a09c0");

    private static final Set<Common.LaunchStatus> TERMINAL_STATES = Set.of(
            Common.LaunchStatus.CANCELED,
            Common.LaunchStatus.FINISHED,
            Common.LaunchStatus.FAILURE
    );

    private final SecurityAccessService securityAccessService;
    private final String ciEnvironment;

    @Nullable
    private final CiClient ciClient;

    @Override
    public UUID getSourceCodeId() {
        return ID;
    }

    @Override
    public void execute(JobContext context) throws Exception {
        new SingleAction(context).execute();
    }

    @Override
    public SecurityAccessService getSecurityAccessService() {
        return securityAccessService;
    }

    // TODO: Переделать. Нет никакого смысла устанавливать новое подключение к CI на каждое действие
    protected CiClient getCiClient(JobContext context, Environment environment) {
        if (ciClient != null) {
            return ciClient;
        }

        LaunchRuntimeInfo runtimeInfo = context.getFlowLaunch().getFlowInfo().getRuntimeInfo();
        String token = fetchCiMainToken(runtimeInfo);

        var properties = GrpcClientProperties.builder()
                .deadlineAfter(Duration.of(2, ChronoUnit.MINUTES))
                .endpoint(environment.apiUrl)
                .userAgent("RunCiAction:" + context.getFlowLaunch().getIdString())
                .callCredentials(new OAuthCallCredentials(token))
                .build();
        return CiClientImpl.create(properties);
    }

    @Nullable
    private Environment getCurrentEnvironment() {
        return switch (ciEnvironment) {
            case CiProfile.STABLE_PROFILE -> Environment.STABLE;
            case CiProfile.TESTING_PROFILE -> Environment.TESTING;
            default -> null;
        };
    }

    enum Environment {
        TESTING("https://arcanum-test.yandex-team.ru", "ci-api.testing.in.yandex-team.ru:4221"),
        STABLE("https://a.yandex-team.ru", "ci-api.in.yandex-team.ru:4221");

        private final String frontendUrl;
        private final String apiUrl;

        Environment(String frontendUrl, String apiUrl) {
            this.frontendUrl = frontendUrl;
            this.apiUrl = apiUrl;
        }
    }

    private class SingleAction {
        private final RunCiAction.Config config;
        private final JobContext context;

        SingleAction(JobContext context) {
            this.config = context.resources().consume(RunCiAction.Config.class);
            this.context = context;
        }

        void execute() throws Exception {
            validateConfig();

            Duration executionTimeout = DurationParser.parse(config.getExecutionTimeout());
            Environment currentEnvironment = getCurrentEnvironment();
            Environment targetEnvironment = getTargetEnvironment(currentEnvironment);
            boolean isSameEnv = (currentEnvironment == targetEnvironment);

            try (CiClient client = getCiClient(context, targetEnvironment)) {
                FrontendOnCommitFlowLaunchApi.FlowLaunch launch = startAction(context, client, isSameEnv);
                try {
                    Common.LaunchStatus result = Poller.poll(
                            () -> getLaunchStatus(context, client, launch, targetEnvironment)
                    )
                            .interval(5L, TimeUnit.SECONDS)
                            .timeout(executionTimeout)
                            .canStopWhen(TERMINAL_STATES::contains)
                            .run();

                    if (result == Common.LaunchStatus.FAILURE) {
                        throw new RuntimeException(
                                "Failed action %s:%s".formatted(config.getActionPath(), config.getActionId()));
                    }
                    log.info("Completed with status {}", result);
                } catch (RuntimeException | TimeoutException e) {
                    context.progress().updateTaskState(
                            getBadge(context, launch, Common.LaunchStatus.FAILURE, targetEnvironment)
                    );
                    throw e;
                }
            }
        }

        private void validateConfig() {
            log.info("Config: {}", config.toString());

            Preconditions.checkArgument(
                    !Strings.isNullOrEmpty(config.getExecutionTimeout()),
                    "config.execution_timeout is required"
            );

            Preconditions.checkArgument(
                    !Strings.isNullOrEmpty(config.getActionPath()),
                    "config.action_path is required"
            );

            Preconditions.checkArgument(
                    !Strings.isNullOrEmpty(config.getActionId()),
                    "config.action_id is required"
            );

        }

        private Common.LaunchStatus getLaunchStatus(JobContext context,
                                                    CiClient client,
                                                    FrontendOnCommitFlowLaunchApi.FlowLaunch launch,
                                                    Environment environment) {
            Common.LaunchStatus flowStatus = fetchStatus(client, launch);
            context.progress().updateTaskState(getBadge(context, launch, flowStatus, environment));
            return flowStatus;
        }

        private Common.LaunchStatus fetchStatus(CiClient client, FrontendOnCommitFlowLaunchApi.FlowLaunch launch) {
            FrontendOnCommitFlowLaunchApi.GetFlowLaunchRequest request =
                    FrontendOnCommitFlowLaunchApi.GetFlowLaunchRequest.newBuilder()
                            .setFlowProcessId(launch.getFlowProcessId())
                            .setNumber(launch.getNumber())
                            .build();
            return client.getFlowStatus(request);
        }

        private TaskBadge getBadge(JobContext context,
                                   FrontendOnCommitFlowLaunchApi.FlowLaunch launch,
                                   Common.LaunchStatus flowStatus,
                                   Environment environment
        ) {
            UrlService service = new UrlService(environment.frontendUrl);
            return TaskBadge.of(
                    TaskBadge.reservedTaskId(TASK_BADGE_ID),
                    "CI_BADGE",
                    service.toFlowLaunch(
                            context.getFlowLaunch().getProjectId(),
                            LaunchId.of(ProtoMappers.toCiProcessId(launch.getFlowProcessId()), launch.getNumber())),
                    getTaskStatus(flowStatus));
        }

        TaskBadge.TaskStatus getTaskStatus(Common.LaunchStatus status) {
            return switch (status) {
                case UNKNOWN, FAILURE, CANCELED -> TaskBadge.TaskStatus.FAILED;
                case FINISHED -> TaskBadge.TaskStatus.SUCCESSFUL;
                default -> TaskBadge.TaskStatus.RUNNING;
            };
        }

        private FrontendOnCommitFlowLaunchApi.FlowLaunch startAction(
                JobContext context,
                CiClient client,
                boolean preserveRevision
        ) {
            OrderedArcRevision targetRevision = context.getFlowLaunch().getTargetRevision();
            var requestBuilder = FrontendOnCommitFlowLaunchApi.StartFlowRequest.newBuilder()
                    .setFlowProcessId(Common.FlowProcessId.newBuilder()
                            .setId(config.getActionId())
                            .setDir(config.getActionPath())
                            .build())
                    .setNotifyPullRequest(false);

            if (preserveRevision || targetRevision.getBranch().isTrunk()) {
                requestBuilder.setBranch(targetRevision.getBranch().getBranch());
            }

            if (preserveRevision) { //If env differs we can't provide rev
                requestBuilder.setRevision(ProtoMappers.toCommitId(targetRevision));
            }

            if (!config.getFlowVars().isEmpty()) {
                requestBuilder.setFlowVars(Common.FlowVars.newBuilder().setJson(config.getFlowVars()));
            }

            FrontendOnCommitFlowLaunchApi.StartFlowResponse response = client.startFlow(requestBuilder.build());
            return response.getLaunch();
        }

        private Environment getTargetEnvironment(@Nullable Environment currentEnvironment) {
            Preconditions.checkState(
                    currentEnvironment != null || config.getEnvironment() != RunCiAction.Config.CiEnvironment.CURRENT,
                    "If config.environment == CURRENT, currentEnvironment can't be null"
            );
            return switch (config.getEnvironment()) {
                case CURRENT -> currentEnvironment;
                case TESTING -> Environment.TESTING;
                case STABLE -> Environment.STABLE;
                case UNRECOGNIZED -> throw new IllegalStateException(
                        "Unrecognized env, with proto number: " + config.getEnvironmentValue()
                );
            };
        }
    }
}
