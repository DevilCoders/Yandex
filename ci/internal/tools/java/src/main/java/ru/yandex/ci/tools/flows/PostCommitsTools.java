package ru.yandex.ci.tools.flows;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Set;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;

import com.google.protobuf.TextFormat;
import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.api.internal.frontend.flow.FrontendFlowApi;
import ru.yandex.ci.api.proto.Common;
import ru.yandex.ci.api.storage.StorageApi;
import ru.yandex.ci.client.ci.CiClient;
import ru.yandex.ci.client.sandbox.SandboxClient;
import ru.yandex.ci.client.sandbox.TasksFilter;
import ru.yandex.ci.client.sandbox.api.SandboxTaskStatus;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.autocheck.AutocheckConstants;
import ru.yandex.ci.core.config.a.model.JobAttemptsSandboxConfig;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.launch.LaunchState;
import ru.yandex.ci.engine.autocheck.AutocheckBootstrapServicePostCommits;
import ru.yandex.ci.engine.config.ConfigurationService;
import ru.yandex.ci.engine.proto.ProtoMappers;
import ru.yandex.ci.engine.spring.AutocheckConfig;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.JobLaunch;
import ru.yandex.ci.flow.engine.runtime.state.model.JobState;
import ru.yandex.ci.storage.core.spring.ClientsConfig;
import ru.yandex.ci.tools.AbstractSpringBasedApp;
import ru.yandex.ci.util.CiJson;
import ru.yandex.ci.util.ResourceUtils;

@Slf4j
@Configuration
@Import({
        ClientsConfig.class,
        AutocheckConfig.class
})
public class PostCommitsTools extends AbstractSpringBasedApp {

    @Autowired
    CiDb db;

    @Autowired
    ConfigurationService configurationService;

    @Autowired
    CiClient ciClient;

    @Autowired
    ArcService arcService;

    @Autowired
    SandboxClient sandboxClient;

    @Override
    protected void run() throws InterruptedException, ExecutionException {
        //
    }

    void markBrokenRevisionsAsComplete() {
        var revisions = Set.of("1b7a52049abd81c59f8fe507b06b764c4a6b5a79");
        for (var revision : revisions) {
            var request = StorageApi.MarkDiscoveredCommitRequest.newBuilder()
                    .setRevision(Common.CommitId.newBuilder()
                            .setCommitId(revision).build())
                    .build();
            log.info("Mark commit on broken revision as processed by storage: {}", revision);
            ciClient.markDiscoveredCommit(request);
        }

    }

    @SuppressWarnings("BusyWait")
    void allStartJobsFinished() throws InterruptedException, ExecutionException {
        var json = ResourceUtils.textResource("flows-to-process.json");
        var chains = CiJson.readValue(json, FlowChains.class);

        log.info("Total chains: {}", chains.flows.size());

        var executor = Executors.newFixedThreadPool(32);

        for (var chain : chains.flows) {
            log.info("Total flows to check: {} [{}]", chain.flowIds.size(), chain.flowIds.get(0));

            log.info("Restarting flows...");
            for (var flowId : chain.flowIds) {
                log.info("Starting flow: {}", flowId);
                try {
                    ciClient.launchJob(FrontendFlowApi.LaunchJobRequest.newBuilder()
                            .setFlowLaunchJobId(Common.FlowLaunchJobId.newBuilder()
                                    .setFlowLaunchId(Common.FlowLaunchId.newBuilder()
                                            .setId(flowId)
                                            .build())
                                    .setJobId("start"))
                            .build());
                } catch (Exception e) {
                    log.error("Unable to start flow {}", flowId, e);
                }
            }

            var flowsToCheck = new LinkedHashSet<>(chain.flowIds);
            while (!flowsToCheck.isEmpty()) {
                var tasks = flowsToCheck.stream()
                        .map(FlowLaunchId::of)
                        .map(id -> executor.submit(() -> loadFlowLaunch(id)))
                        .toList();

                for (var future : tasks) {
                    var response = future.get();
                    if (isFlowLaunchComplete(response)) {
                        var flowId = response.getIdString();
                        flowsToCheck.remove(response.getIdString());
                        log.info("Check complete: {}", flowId);
                    }
                }

                if (!flowsToCheck.isEmpty()) {
                    for (var flowId : flowsToCheck) {
                        log.info("{}", flowId);
                    }
                    log.info("Checking still incomplete: {}", flowsToCheck.size());
                    Thread.sleep(1000);
                }
            }

            log.info("All flows are complete, launch next part");
        }
    }

    private FlowLaunchEntity loadFlowLaunch(FlowLaunchId flowLaunchId) {
        return db.currentOrReadOnly(() -> db.flowLaunch().get(flowLaunchId));
    }

    private boolean isFlowLaunchComplete(FlowLaunchEntity flowLaunchEntity) {
        return flowLaunchEntity.getJobs()
                .get("start")
                .getLaunches()
                .stream()
                .max(Comparator.comparing(JobLaunch::getNumber))
                .orElseThrow()
                .getLastStatusChange()
                .getType()
                .isSucceeded();
    }

    void stopDuplicateFlows() {
        var processId = AutocheckBootstrapServicePostCommits.TRUNK_POSTCOMMIT_PROCESS_ID;

        var json = ResourceUtils.textResource("duplicate-flows.json");
        var duplicates = CiJson.readValue(json, Duplicates.class);

        log.info("Total duplicates: {}", duplicates.list.size());

        for (var flow : duplicates.list) {

            boolean skipCancel = db.currentOrTx(() -> {
                var launch = db.launches().get(LaunchId.of(processId, flow.launchNumber));
                if (launch.getStatus() == LaunchState.Status.POSTPONE) {
                    log.info("Skip POSTPONE launch for r{}, {}", flow.revision, flow.launchNumber);
                    db.launches().save(launch.withStatus(LaunchState.Status.IDLE));
                    return true;
                }
                return false;
            });

            if (skipCancel) {
                continue;
            }

            var request = StorageApi.CancelFlowRequest.newBuilder()
                    .setFlowProcessId(ProtoMappers.toProtoFlowProcessId(processId))
                    .setNumber(flow.launchNumber)
                    .build();
            log.info("Stopping flow at r{}: {}", flow.revision, TextFormat.shortDebugString(request));

            ciClient.cancelFlow(request);
        }
    }

    @SuppressWarnings({"ResultOfMethodCallIgnored", "FutureReturnValueIgnored"})
    void stopObsoleteSandboxTasks() throws InterruptedException {
        var processId = AutocheckBootstrapServicePostCommits.TRUNK_POSTCOMMIT_PROCESS_ID;
        var launchIds = db.currentOrReadOnly(() -> db.launches()
                .getLaunchIds(processId, ArcBranch.trunk(), LaunchState.Status.RUNNING));
        log.info("Total launches: {}", launchIds.size());

        var executor = Executors.newFixedThreadPool(32);

        var tasksToStop = Collections.synchronizedList(new ArrayList<Long>());
        for (var launchId : launchIds) {
            executor.submit(() -> {
                var flowLaunch = db.currentOrReadOnly(() ->
                        db.flowLaunch().get(FlowLaunchId.of(LaunchId.fromKey(launchId))));

                var revision = flowLaunch.getVcsInfo().getRevision().getNumber();
                log.info("Launch: {}, revision: {}", launchId, revision);

                Set<Long> keepTasks = new HashSet<>();
                for (var job : flowLaunch.getJobs().values()) {
                    Long lastTaskId = null;
                    for (var launch : job.getLaunches()) {
                        for (var badge : launch.getTaskStates()) {
                            if ("SANDBOX".equals(badge.getModule()) && badge.getUrl() != null) {
                                var taskIdString = badge.getUrl()
                                        .substring("https://sandbox.yandex-team.ru/task/".length());
                                lastTaskId = Long.parseLong(taskIdString);
                            }
                        }
                    }
                    if (lastTaskId != null) {
                        keepTasks.add(lastTaskId);
                    }
                }

                log.info("Keep: {}", keepTasks);

                var tasks = sandboxClient.getTasks(TasksFilter.builder()
                        .type(AutocheckConstants.AUTOCHECK_BUILD_PARENT_2)
                        .hidden(true)
                        .children(false)
                        .hint(String.valueOf(revision))
                        .field("id,author,tags,status")
                        .offset(0)
                        .limit(1000)
                        .build()
                );

                for (var task : tasks) {
                    if (task.getStatus().isRunning() &&
                            task.getStatus() != SandboxTaskStatus.DRAFT &&
                            !keepTasks.contains(task.getId())) {
                        sandboxClient.stopTask(task.getId(), "Stop task without active flow");
                        tasksToStop.add(task.getId());
                    }
                }
            });
        }

        executor.shutdown();
        executor.awaitTermination(30, TimeUnit.MINUTES);

        log.info("Total tasks to stop: {}", tasksToStop.size());
        for (var taskId : tasksToStop) {
            log.info("Sandbox task: {}", taskId);
        }
    }

    @SuppressWarnings({"ResultOfMethodCallIgnored", "FutureReturnValueIgnored"})
    void updatePostponeLaunchesConfigVersion() throws InterruptedException {
        var ciProcessId = AutocheckBootstrapServicePostCommits.TRUNK_POSTCOMMIT_PROCESS_ID;
        var launches = db.currentOrReadOnly(() ->
                db.launches().getLaunchIds(ciProcessId, ArcBranch.trunk(), LaunchState.Status.POSTPONE));

        var executor = Executors.newFixedThreadPool(32);

        var latestConfig = db.currentOrReadOnly(() ->
                db.configHistory().findLastConfig(ciProcessId.getPath(), ArcBranch.trunk()))
                .orElseThrow(() -> new RuntimeException("Unable to find latest config for " + ciProcessId));

        log.info("Latest config: {}", latestConfig);

        var total = new AtomicInteger();
        for (var launchId : launches) {
            executor.submit(() -> {
                db.currentOrTx(() -> {
                    var launch = db.launches().get(launchId);
                    if (launch.getStatus() == LaunchState.Status.POSTPONE) {
                        var config = launch.getFlowInfo().getConfigRevision();
                        if (!config.equals(latestConfig.getRevision())) {
                            var launchToSave = launch.toBuilder()
                                    .flowInfo(launch.getFlowInfo().toBuilder()
                                            .configRevision(latestConfig.getRevision())
                                            .build())
                                    .build();
                            log.info("Updating launch {}", launch.getId());
                            total.incrementAndGet();
                            db.launches().save(launchToSave);
                        }
                    }
                });
            });
        }

        executor.shutdown();
        executor.awaitTermination(30, TimeUnit.MINUTES);

        log.info("Total launches updated: {}", total.get());
    }

    void updateLaunchesWithIdleToCanceled() {
        var ciProcessId = AutocheckBootstrapServicePostCommits.TRUNK_POSTCOMMIT_PROCESS_ID;
        var launches = db.currentOrReadOnly(() ->
                db.launches().getLaunchIds(ciProcessId, ArcBranch.trunk(), LaunchState.Status.IDLE));

        var total = new ArrayList<Launch.Id>(launches.size());
        for (var launchId : launches) {
            db.currentOrTx(() -> {
                var launch = db.launches().get(launchId);
                if (launch.getStatus() == LaunchState.Status.IDLE) {
                    var launchToSave = launch.withStatus(LaunchState.Status.CANCELED);
                    log.info("Updating launch {}", launch.getId());
                    total.add(launchId);
                    db.launches().save(launchToSave);
                }
            });
        }

        for (var launchId : total) {
            log.info("Launch CANCELED: {}", launchId);
        }
    }

    @SuppressWarnings({"ResultOfMethodCallIgnored", "FutureReturnValueIgnored"})
    void updateFlowLaunchesWithAttempts() throws InterruptedException {
        var ciProcessId = AutocheckBootstrapServicePostCommits.TRUNK_POSTCOMMIT_PROCESS_ID;
        var launches = db.currentOrReadOnly(() ->
                db.launches().getLaunchIds(ciProcessId, ArcBranch.trunk(), LaunchState.Status.RUNNING));

        var executor = Executors.newFixedThreadPool(32);

        var latestConfig = db.currentOrReadOnly(() ->
                db.configHistory().findLastConfig(ciProcessId.getPath(), ArcBranch.trunk()))
                .orElseThrow(() -> new RuntimeException("Unable to find latest config for " + ciProcessId));

        log.info("Latest config: {}", latestConfig);

        var updates = Collections.synchronizedList(new ArrayList<Update>());
        for (var launchId : launches) {
            executor.submit(() -> {
                db.currentOrTx(() -> {
                    var launch = db.launches().get(launchId);
                    if (launch.getStatus() == LaunchState.Status.RUNNING) {
                        var config = launch.getFlowInfo().getConfigRevision();
                        if (!config.equals(latestConfig.getRevision())) {

                            boolean updated = false;
                            var flowLaunch = db.flowLaunch().get(FlowLaunchId.of(LaunchId.fromKey(launchId)));
                            var jobs = new LinkedHashMap<String, JobState>(flowLaunch.getJobs().size());
                            for (var e : flowLaunch.getJobs().entrySet()) {

                                var id = e.getKey();
                                var job = e.getValue();

                                var retry = job.getRetry();
                                if (retry == null) {
                                    jobs.put(id, job);
                                    continue;
                                }

                                if (retry.getSandboxConfig() == null) {
                                    var newRetry = retry.toBuilder()
                                            .sandboxConfig(JobAttemptsSandboxConfig.builder()
                                                    .reuseTasks(true)
                                                    .build())
                                            .build();
                                    job.setRetry(newRetry);
                                    updated = true;
                                } else {
                                    var newRetry = retry.toBuilder()
                                            .sandboxConfig(retry.getSandboxConfig().withReuseTasks(true))
                                            .build();
                                    job.setRetry(newRetry);
                                    updated = true;
                                }

                                jobs.put(id, job);
                            }

                            if (updated) {
                                var launchToSave = launch.toBuilder()
                                        .flowInfo(launch.getFlowInfo().toBuilder()
                                                .configRevision(latestConfig.getRevision())
                                                .build())
                                        .build();

                                var flowToSave = flowLaunch.toBuilder()
                                        .clearJobs()
                                        .jobs(jobs)
                                        .flowInfo(
                                                flowLaunch.getFlowInfo().toBuilder()
                                                        .configRevision(latestConfig.getRevision())
                                                        .build())
                                        .build();

                                db.launches().save(launchToSave);
                                db.flowLaunch().save(flowToSave);

                                updates.add(new Update(launch.getId(), flowLaunch.getId()));
                            }
                        }
                    }
                });
            });
        }

        executor.shutdown();
        executor.awaitTermination(30, TimeUnit.MINUTES);

        log.info("Total launches updated: {}", updates.size());
        for (var update : updates) {
            log.info("{}", update);
        }
    }

    // launch the method with something like `-Xms10G -Xmx12G`
    public static void main(String[] args) {
        startAndStopThisClass(args);
    }


    @lombok.Value
    private static class FlowChain {
        List<String> flowIds;
    }

    @lombok.Value
    private static class FlowChains {
        List<FlowChain> flows;
    }

    @lombok.Value
    private static class Reference {
        int revision;
        int launchNumber;
    }

    @lombok.Value
    private static class Duplicates {
        List<Reference> list;
    }

    @lombok.Value
    private static class Update {
        Launch.Id launchId;
        FlowLaunchEntity.Id flowLaunchId;
    }

}
