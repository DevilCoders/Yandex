package ru.yandex.ci.engine;

import java.nio.file.Path;
import java.time.Duration;
import java.time.Instant;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.TimeUnit;
import java.util.function.Function;
import java.util.function.Predicate;
import java.util.function.Supplier;

import javax.annotation.Nonnull;

import com.google.errorprone.annotations.CanIgnoreReturnValue;
import lombok.AllArgsConstructor;
import lombok.RequiredArgsConstructor;
import lombok.Value;
import org.mockito.Mockito;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import ru.yandex.ci.client.sandbox.SandboxClient;
import ru.yandex.ci.client.sandbox.api.DelegationResultList;
import ru.yandex.ci.client.yav.YavClient;
import ru.yandex.ci.client.yav.model.DelegatingTokenResponse;
import ru.yandex.ci.client.yav.model.YavResponse;
import ru.yandex.ci.client.yav.model.YavSecret;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.launch.LaunchState.Status;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.config.ConfigurationService;
import ru.yandex.ci.engine.flow.SecurityDelegationService;
import ru.yandex.ci.engine.flow.YavDelegationException;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.helpers.FlowTestQueries;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.JobLaunch;
import ru.yandex.ci.flow.engine.runtime.state.model.JobState;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChangeType;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;

@RequiredArgsConstructor
public class EngineTester {
    private static final Logger log = LoggerFactory.getLogger(EngineTester.class);

    @Nonnull
    private final YavClient yavClient;
    @Nonnull
    private final SecurityDelegationService securityDelegationService;
    @Nonnull
    private final SandboxClient securitySandboxClient;
    @Nonnull
    private final ConfigurationService configurationService;
    @Nonnull
    private final CiMainDb db;
    @Nonnull
    private final FlowTestQueries flowTestQueries;

    public void delegateToken(Path configPath) throws YavDelegationException {
        delegateToken(configPath, ArcBranch.trunk());
    }

    public void delegateToken(
            Path configPath,
            ArcBranch trunk,
            YavSecret.KeyValue... keys
    ) throws YavDelegationException {
        log.info("Delegating token for {}", configPath);
        mockYav();

        securityDelegationService.delegateYavTokens(
                configurationService.getLastValidConfig(configPath, trunk),
                TestData.USER_TICKET,
                TestData.CI_USER
        );

        var secrets = new ArrayList<YavSecret.KeyValue>();
        secrets.add(new YavSecret.KeyValue(YavSecret.CI_TOKEN, "ci-token"));
        secrets.addAll(List.of(keys));
        Mockito.when(yavClient.getSecretByDelegatingToken(anyString(), any(), eq(configPath.toString())))
                .thenReturn(new YavSecret(YavResponse.Status.OK, "ok", "ok", "version", secrets));

    }

    public void mockYav() {
        Mockito.when(
                yavClient.createDelegatingToken(
                        Mockito.eq(TestData.USER_TICKET),
                        Mockito.eq(TestData.SECRET),
                        anyString(),
                        anyString()
                )).thenReturn(
                new DelegatingTokenResponse(
                        YavResponse.Status.OK, "code", "ok", "token", TestData.YAV_TOKEN_UUID.getId()
                )
        );

        Mockito.when(
                yavClient.createDelegatingToken(
                        Mockito.eq(TestData.USER_TICKET_2),
                        Mockito.eq(TestData.SECRET),
                        anyString(),
                        anyString()
                )).thenReturn(
                new DelegatingTokenResponse(
                        YavResponse.Status.OK, "code", "ok", "token", TestData.YAV_TOKEN_UUID_2.getId()
                )
        );

        Mockito.when(securitySandboxClient.delegateYavSecret(TestData.SECRET, TestData.USER_TICKET))
                .thenReturn(new DelegationResultList.DelegationResult(true, null, null));

        Mockito.when(securitySandboxClient.delegateYavSecret(TestData.SECRET, TestData.USER_TICKET_2))
                .thenReturn(new DelegationResultList.DelegationResult(true, null, null));

    }

    @SuppressWarnings("BusyWait")
    private <T> T wait(Duration duration,
                       String waitingMessage,
                       Supplier<String> errorMessageSupplier,
                       Supplier<T> retryOperation) throws InterruptedException {
        Instant start = Instant.now();
        while (true) {
            T result = retryOperation.get();

            if (result != null) {
                return result;
            }

            if (Duration.between(start, Instant.now()).compareTo(duration) > 0) {
                throw new RuntimeException(errorMessageSupplier.get() + " in " + duration.toSeconds() + "sec");
            }

            log.info(waitingMessage);
            Thread.sleep(TimeUnit.SECONDS.toMillis(5));
        }
    }

    @CanIgnoreReturnValue
    public Launch waitLaunch(LaunchId launchId, Duration duration) throws InterruptedException {
        return waitLaunch(launchId, duration,
                new LaunchPredicate(
                        "Launch status is still 'processing'",
                        launch -> !launch.getStatus().isProcessing())
        );
    }

    @CanIgnoreReturnValue
    public Launch waitLaunchFlowStarted(LaunchId launchId, Duration duration) throws InterruptedException {
        return waitLaunch(launchId, duration,
                new LaunchPredicate(
                        "Launch flowLaunch is still null",
                        launch -> launch.getFlowLaunchId() != null)
        );
    }

    public Launch waitLaunchAnyStatus(LaunchId launchId, Duration duration) throws InterruptedException {
        return waitLaunch(launchId, duration,
                new LaunchPredicate(
                        "Launch not found",
                        launch -> true
                )
        );
    }

    public Launch waitLaunch(LaunchId launchId, Duration duration, Status status) throws InterruptedException {
        return waitLaunch(launchId, duration,
                new LaunchPredicate(
                        launch -> "Launch status is not " + status + ", it is " + launch.getStatus(),
                        launch -> launch.getStatus() == status)
        );
    }

    public Launch waitLaunch(LaunchId launchId, Duration duration, LaunchPredicate checkFinish)
            throws InterruptedException {
        return wait(duration,
                "Waiting " + launchId + "...",
                () ->
                        db.currentOrReadOnly(() -> {
                            var launchOptional = db.launches().findOptional(launchId);
                            if (launchOptional.isEmpty()) {
                                return "Launch " + launchId + " wasn't found";
                            }
                            var launch = launchOptional.get();

                            var sb = new StringBuilder("Launch don't become in final state: " +
                                    checkFinish.errorMessage(launch));

                            sb.append("\nlaunch:\n").append(launch);
                            var flowLaunch =
                                    flowTestQueries.getFlowLaunchOptional(FlowLaunchId.of(launchId)).orElse(null);
                            sb.append("\nflow:\n").append(flowLaunch);
                            return sb.toString();
                        }),
                () -> {

                    var launchOptional = db.currentOrReadOnly(() -> db.launches().findOptional(launchId));
                    if (launchOptional.isEmpty()) {
                        log.info("Launch {} wasn't created yet", launchId);
                        return null;
                    }
                    var launch = launchOptional.get();
                    log.info("Checking launch {}, status: {}", launchId, launch.getStatus());

                    if (checkFinish.test(launch)) {
                        log.info("Launch in {} state, stop waiting", launch.getStatus());
                        return launch;
                    }

                    return null;
                });
    }

    public JobInfo waitJob(LaunchId launchId, Duration duration, String jobId, StatusChangeType status)
            throws InterruptedException {
        return wait(duration, "Waiting " + launchId + " for job " + jobId,
                () -> "Job " + jobId + " of launch " + launchId + " don't became in state " + status,
                () -> {
                    Launch launch = db.currentOrReadOnly(() -> db.launches().get(launchId));
                    String flowLaunchId = launch.getFlowLaunchId();
                    if (flowLaunchId == null) {
                        log.info("Launch {} doesn't have flowLaunchId", launchId);
                        return null;
                    }

                    FlowLaunchEntity flowLaunch =
                            flowTestQueries.getFlowLaunch(FlowLaunchId.of(launch.getFlowLaunchId()));
                    if (!flowLaunch.getJobs().containsKey(jobId)) {
                        log.info("Launch {} doesn't have job {}", launchId, jobId);
                        return null;
                    }

                    JobState jobState = flowLaunch.getJobState(jobId);
                    JobLaunch lastLaunch = jobState.getLastLaunch();
                    if (lastLaunch == null) {
                        log.info("Launch {} job {} doesn't have launch", launchId, jobId);
                        return null;
                    }

                    StatusChangeType actual = lastLaunch.getLastStatusChange().getType();
                    if (actual != status) {
                        log.info("Launch {} job {} has state {}, need {}", launchId, jobId, actual, status);
                        return null;
                    }
                    return JobInfo.of(launch, jobState);
                }
        );
    }

    @Value(staticConstructor = "of")
    public static class JobInfo {
        @Nonnull
        Launch launch;
        @Nonnull
        JobState jobState;
    }

    @Value
    @AllArgsConstructor
    public static class LaunchPredicate implements Predicate<Launch> {
        @Nonnull
        Function<Launch, String> errorMessage;
        @Nonnull
        Predicate<Launch> delegate;

        public LaunchPredicate(String errorMessage, Predicate<Launch> delegate) {
            this.errorMessage = unused -> errorMessage;
            this.delegate = delegate;
        }

        @Override
        public boolean test(Launch launch) {
            return delegate.test(launch);
        }

        public String errorMessage(Launch launch) {
            return errorMessage.apply(launch);
        }
    }
}
