package ru.yandex.ci.tools.autocheck;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;

import com.google.protobuf.TextFormat;
import lombok.extern.slf4j.Slf4j;
import org.slf4j.MDC;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.api.storage.StorageApi;
import ru.yandex.ci.client.ci.CiClient;
import ru.yandex.ci.client.storage.StorageApiClient;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.discovery.CommitDiscoveryProgress;
import ru.yandex.ci.core.discovery.DiscoveryType;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.launch.LaunchState;
import ru.yandex.ci.engine.autocheck.AutocheckService;
import ru.yandex.ci.engine.autocheck.jobs.autocheck.StartAutocheckJob;
import ru.yandex.ci.engine.proto.ProtoMappers;
import ru.yandex.ci.engine.spring.AutocheckConfig;
import ru.yandex.ci.engine.spring.ConfigurationServiceConfig;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.spring.ydb.YdbCiConfig;
import ru.yandex.ci.storage.core.CheckOuterClass;
import ru.yandex.ci.storage.core.spring.ClientsConfig;
import ru.yandex.ci.tools.AbstractSpringBasedApp;

import static ru.yandex.ci.engine.autocheck.AutocheckBootstrapServicePostCommits.TRUNK_POSTCOMMIT_PROCESS_ID;

@Slf4j
@Configuration
@Import({
        YdbCiConfig.class,
        ConfigurationServiceConfig.class,
        ClientsConfig.class,
        AutocheckConfig.class
})
public class SkipPostCommit extends AbstractSpringBasedApp {

    @Autowired
    CiDb db;

    @Autowired
    CiClient ciClient;

    @Autowired
    AutocheckService autocheckService;

    @Autowired
    StorageApiClient storageApiClient;

    @Override
    protected void run() throws Exception {
        // skipWithExecutionExceptionStacktrace();
    }

    void skipWithExecutionExceptionStacktrace() throws InterruptedException {
        var executionExceptionStacktrace = "ru.yandex.ci.flow.engine.runtime.state.calculator" +
                ".MultiplicationRuntimeException: Unable to substitute field \"autocheck_revision\" with expression " +
                "\"${tasks.start.launch.check_info.distbuild_priority.priority_revision}\": No value resolved for " +
                "expression: tasks.start.launch.check_info.distbuild_priority.priority_revision\n";

        var launches = findFlowsWithExecutionExceptionStacktrace(executionExceptionStacktrace);
        stopAndSkipCommits(launches);
    }

    @SuppressWarnings({"ResultOfMethodCallIgnored", "FutureReturnValueIgnored"})
    private List<Launch> findFlowsWithExecutionExceptionStacktrace(
            String executionExceptionStacktrace) throws InterruptedException {

        var launchStatuses = List.of(
                LaunchState.Status.RUNNING,
                LaunchState.Status.WAITING_FOR_SCHEDULE
        );

        var launchIds = db.currentOrReadOnly(() ->
                launchStatuses.stream()
                        .map(status -> db.launches().getLaunchIds(
                                TRUNK_POSTCOMMIT_PROCESS_ID, ArcBranch.trunk(), status
                        ))
                        .flatMap(Collection::stream)
                        .toList()
        );

        var executor = Executors.newFixedThreadPool(32);
        var launches = Collections.synchronizedList(new ArrayList<Launch>());
        var progress = new AtomicInteger();
        for (var launchId : launchIds) {
            executor.submit(() -> db.currentOrReadOnly(() -> {
                var launch = db.launches().get(launchId);
                var flowLaunch = db.flowLaunch().get(FlowLaunchId.of(LaunchId.fromKey(launchId)));

                if (!flowLaunch.isDisabled() && !flowLaunch.isDisablingGracefully()) {
                    jobs_loop:
                    for (var job : flowLaunch.getJobs().values()) {
                        for (var jobLaunch : job.getLaunches()) {
                            var stacktrace = jobLaunch.getExecutionExceptionStacktrace();
                            if (stacktrace != null && stacktrace.contains(executionExceptionStacktrace)) {
                                launches.add(launch);
                                break jobs_loop;
                            }
                        }
                    }
                }

                log.info("Processed launches {}/{}", progress.incrementAndGet(), launchIds.size());
            }));
        }

        executor.shutdown();
        executor.awaitTermination(30, TimeUnit.MINUTES);

        log.info("Found launches: {}", launches.size());
        return launches;
    }

    private void stopAndSkipCommits(List<Launch> launches) {
        var cancelledLaunchNumbers = new HashSet<Integer>();

        for (var launch : launches) {
            var launchNumber = launch.getLaunchId().getNumber();
            var revision = launch.getVcsInfo().getRevision();

            try {
                MDC.put("launchNumber", String.valueOf(launchNumber));
                MDC.put("flowLaunchId", launch.getFlowLaunchId());
                MDC.put("targetCommit", revision.getCommitId());

                if (!cancelledLaunchNumbers.add(launchNumber)) {
                    log.info("Skipped");
                    continue;
                }

                try {
                    log.info("Found");

                    log.info("Cancelling check");
                    cancelCheck(launch);
                    log.info("Canceled check");

                    log.info("Cancelling flow");
                    cancelFlow(launch.getId().getLaunchNumber());
                    log.info("Canceled flow");

                    log.info("Marking as discovered");
                    db.currentOrTx(() -> {
                        var updatedProgress = db.commitDiscoveryProgress()
                                .find(revision.getCommitId())
                                .map(it -> it.withDiscoveryFinished(DiscoveryType.STORAGE))
                                .orElseGet(() -> CommitDiscoveryProgress.of(revision)
                                        .withDiscoveryFinished(DiscoveryType.STORAGE)
                                );
                        db.commitDiscoveryProgress().save(updatedProgress);
                    });
                    log.info("Marked as discovered");
                } catch (Exception e) {
                    log.error("Failed to restart", e);
                }
            } finally {
                MDC.remove("launchNumber");
                MDC.remove("flowLaunchId");
                MDC.remove("targetCommit");
            }
        }
    }

    void cancelCheck(Launch launch) {
        var vcsInfo = launch.getVcsInfo();
        var leftCommitId = autocheckService.getLeftRevisionForAutocheckPostcommits(vcsInfo).getCommitId();
        var rightRevision = StartAutocheckJob.getRightRevision(vcsInfo);

        var request = ru.yandex.ci.storage.api.StorageApi.FindCheckByRevisionsRequest.newBuilder()
                .setLeftRevision(leftCommitId)
                .setRightRevision(rightRevision.getCommitId())
                .addAllTags(List.of(
                        String.valueOf(rightRevision.getNumber()),
                        "postcommit"
                ))
                .build();

        var checkIds = storageApiClient.findChecksByRevisionsAndTags(request)
                .getChecksList()
                .stream()
                .map(CheckOuterClass.Check::getId)
                .toList();

        if (checkIds.size() == 1) {
            log.info("Found check ids {}: {}", checkIds, request);
        } else {
            log.warn("Found check ids {} size != 1: {}", checkIds, request);
        }

        if (checkIds.isEmpty()) {
            return;
        }

        for (var id : checkIds) {
            log.info("Cancelling check {}", id);
            storageApiClient.cancelCheck(id);
        }
    }

    private void cancelFlow(int launchNumber) {
        var request = StorageApi.CancelFlowRequest.newBuilder()
                .setFlowProcessId(ProtoMappers.toProtoFlowProcessId(TRUNK_POSTCOMMIT_PROCESS_ID))
                .setNumber(launchNumber)
                .build();
        log.info("Cancelling flow flowNumber {}: {}", launchNumber, TextFormat.shortDebugString(request));

        ciClient.cancelFlow(request);
    }

    // launch the method with something like `-Xms10G -Xmx12G`
    public static void main(String[] args) {
        startAndStopThisClass(args);
    }
}
