package ru.yandex.ci.engine.discovery.tier0;

import java.time.Clock;
import java.time.Instant;
import java.time.temporal.ChronoUnit;
import java.util.ArrayList;
import java.util.List;
import java.util.Optional;
import java.util.Set;
import java.util.TreeSet;
import java.util.concurrent.atomic.AtomicLong;
import java.util.function.Consumer;
import java.util.function.Function;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import lombok.AllArgsConstructor;
import lombok.Value;
import lombok.extern.slf4j.Slf4j;

import yandex.cloud.repository.db.list.ListRequest;
import ru.yandex.ci.client.sandbox.SandboxClient;
import ru.yandex.ci.client.sandbox.api.BatchResult;
import ru.yandex.ci.client.sandbox.api.BatchStatus;
import ru.yandex.ci.client.sandbox.api.SandboxTaskOutput;
import ru.yandex.ci.client.sandbox.api.SandboxTaskPriority;
import ru.yandex.ci.client.sandbox.api.SandboxTaskStatus;
import ru.yandex.ci.client.sandbox.api.TaskExecution;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.discovery.DiscoveryType;
import ru.yandex.ci.core.discovery.GraphDiscoveryTask;
import ru.yandex.ci.engine.autocheck.AutocheckBlacklistService;
import ru.yandex.ci.engine.launch.auto.DiscoveryProgressService;
import ru.yandex.ci.util.UserUtils;
import ru.yandex.commune.bazinga.BazingaTaskManager;
import ru.yandex.lang.NonNullApi;

import static ru.yandex.ci.core.discovery.GraphDiscoveryTask.Status.GRAPH_EVALUATION_FAILED;
import static ru.yandex.ci.core.discovery.GraphDiscoveryTask.Status.GRAPH_EVALUATION_STARTED;
import static ru.yandex.ci.core.discovery.GraphDiscoveryTask.Status.GRAPH_EVALUATION_SUCCESS;

@Slf4j
@NonNullApi
@AllArgsConstructor
public class GraphDiscoveryService {

    private static final String DEFAULT_COMMENT_FOR_SANDBOX_TASK =
            "Task started by " + UserUtils.loginForInternalCiProcesses();

    private static final int ATTEMPTS_NUMBER_THRESHOLD = 5;
    private static final String NS_GRAPH_DISCOVERY_SERVICE = "GraphDiscoveryService";

    private final DiscoveryProgressService discoveryProgressService;
    private final CiMainDb db;
    private final SandboxClient sandboxClient;
    private final BazingaTaskManager bazingaTaskManager;
    private final Clock clock;
    private final GraphDiscoveryServiceProperties properties;
    private final AutocheckBlacklistService autocheckBlacklistService;

    private final AtomicLong startedTaskCount = new AtomicLong(0);
    private final AtomicLong processedResultCount = new AtomicLong(0);

    public void scheduleGraphDiscovery(@Nullable ArcCommit leftCommit, ArcCommit rightCommit,
                                       OrderedArcRevision rightRevision) {
        if (isSkipCommit(leftCommit, rightRevision)) {
            markAsDiscovered(rightRevision);
            return;
        }

        bazingaTaskManager.schedule(new GraphDiscoveryStartTask(
                new GraphDiscoveryStartTask.Params(
                        rightCommit.getCommitId(),
                        leftCommit.getCommitId(),
                        rightCommit.getSvnRevision(),
                        leftCommit.getSvnRevision(),
                        Set.of(
                                GraphDiscoveryTask.Platform.LINUX,
                                GraphDiscoveryTask.Platform.MANDATORY
                        )
                )
        ));
    }

    public void markAsDiscovered(OrderedArcRevision rightRevision) {
        discoveryProgressService.markAsDiscovered(rightRevision, DiscoveryType.GRAPH);
    }

    private boolean isSkipCommit(
            @Nullable ArcCommit leftCommit,
            OrderedArcRevision rightRevision
    ) {
        var branch = rightRevision.getBranch();

        if (!properties.isEnabled()) {
            log.info("Skipped {} at {} cause discovery is disabled", rightRevision.getCommitId(), branch);
            return true;
        }

        if (!branch.isTrunk()) {
            log.info("Skipped {} at {} cause branch is not trunk", rightRevision.getCommitId(), branch);
            return true;
        }

        if (leftCommit == null) {
            log.info("Skipped {} at {} cause left commit is null", rightRevision.getCommitId(), branch);
            return true;
        }

        var blacklisted = autocheckBlacklistService.isOnlyBlacklistPathsAffected(rightRevision.toRevision());
        if (blacklisted) {
            log.info("Skipped {} at {} cause all affected paths are in blacklist", rightRevision.getCommitId(), branch);
            return true;
        }

        return false;
    }

    public void startGraphDiscoverySandboxTask(
            ArcRevision rightRevision,
            ArcRevision leftRevision,
            long rightSvnRevision,
            long leftSvnRevision,
            Set<GraphDiscoveryTask.Platform> platforms
    ) {
        try {
            var tasks = db.currentOrReadOnly(() ->
                    db.graphDiscoveryTaskTable().findByArcRevision(rightRevision)
            );

            var targetPlatforms = new TreeSet<>(platforms);
            var draftPlatforms = tasks.stream()
                    .filter(task -> SandboxTaskStatus.DRAFT.name().equals(task.getSandboxStatus()))
                    .map(GraphDiscoveryTask::getId)
                    .map(GraphDiscoveryTask.Id::getPlatforms)
                    .flatMap(Set::stream)
                    .filter(platforms::contains)
                    .collect(Collectors.toSet());
            targetPlatforms.removeAll(draftPlatforms);

            var startedPlatforms = tasks.stream()
                    .filter(task -> !SandboxTaskStatus.DRAFT.name().equals(task.getSandboxStatus()))
                    .map(GraphDiscoveryTask::getId)
                    .map(GraphDiscoveryTask.Id::getPlatforms)
                    .flatMap(Set::stream)
                    .filter(platforms::contains)
                    .collect(Collectors.toSet());
            targetPlatforms.removeAll(startedPlatforms);

            log.info("Requested graph discovery for (right: {} ({}), left: {} ({}), {}), " +
                            "already started {}, in draft state {}",
                    rightSvnRevision, rightRevision, leftSvnRevision, leftRevision, platforms, startedPlatforms,
                    draftPlatforms);

            var draftSandboxTask = new ArrayList<GraphDiscoveryTask>();
            if (!targetPlatforms.isEmpty()) {
                var createdTask = createTaskInSandbox(rightRevision, leftRevision, rightSvnRevision, leftSvnRevision,
                        targetPlatforms);
                draftSandboxTask.add(createdTask);
            }

            tasks.stream()
                    .filter(task -> SandboxTaskStatus.DRAFT.name().equals(task.getSandboxStatus()))
                    .forEach(draftSandboxTask::add);
            startTasksInSandbox(draftSandboxTask, false);
            startedTaskCount.addAndGet(draftSandboxTask.size());
        } catch (GraphDiscoveryException e) {
            throw e;
        } catch (Exception e) {
            throw new GraphDiscoveryException(e);
        }
    }

    public void restartGraphDiscoverySandboxTask(List<GraphDiscoveryTask> tasks) {
        try {
            var attemptsNumberThreshold = getAttemptsNumberThreshold();
            var restartTaskContext = new RestartTaskContext(tasks, attemptsNumberThreshold);
            startTasksInSandbox(restartTaskContext.restartTask, true);
            log.info("Tasks restarted {}", getSandboxTaskIds(restartTaskContext.restartTask));

            var recreatedTasks = restartTaskContext.createNewTask.stream()
                    .map(task -> {
                        var sandboxTaskOutput = sendTaskCreationRequestToSandbox(
                                task.getRightRevision(),
                                task.getLeftRevision(),
                                task.getRightSvnRevision(),
                                task.getLeftSvnRevision(),
                                task.getId().getPlatforms()
                        );
                        log.info("Created new task {} as a replace for {}", sandboxTaskOutput.getId(), task.getId());
                        return Pair.of(task, sandboxTaskOutput);
                    })
                    .map(pair ->
                            db.currentOrTx(() -> {
                                var task = pair.task;
                                var sandboxResponse = pair.getSandboxResponse();
                                var updatedTask = task.toBuilder()
                                        .status(GRAPH_EVALUATION_STARTED)
                                        .sandboxTaskId(sandboxResponse.getId())
                                        .sandboxStatus(sandboxResponse.getStatus().name())
                                        .attemptNumber(task.getAttemptNumber() + 1)
                                        .failedSandboxTaskId(task.getSandboxTaskId())
                                        .build();
                                return db.graphDiscoveryTaskTable().save(updatedTask);
                            })
                    )
                    .collect(Collectors.toList());

            log.info("Starting recreated tasks {}", getSandboxTaskIds(recreatedTasks));
            startTasksInSandbox(recreatedTasks, false);
            log.info("Started recreated tasks {}", getSandboxTaskIds(recreatedTasks));
        } catch (GraphDiscoveryException e) {
            throw e;
        } catch (Exception e) {
            throw new GraphDiscoveryException(e);
        }
    }

    private int getAttemptsNumberThreshold() {
        return db.currentOrReadOnly(() -> db.keyValue().getInt(
                NS_GRAPH_DISCOVERY_SERVICE, "attemptsNumberThreshold", ATTEMPTS_NUMBER_THRESHOLD
        ));
    }

    private GraphDiscoveryTask createTaskInSandbox(
            ArcRevision rightRevision,
            ArcRevision leftRevision,
            long rightSvnRevision,
            long leftSvnRevision,
            Set<GraphDiscoveryTask.Platform> platforms
    ) {
        var sandboxResponse = sendTaskCreationRequestToSandbox(
                rightRevision, leftRevision, rightSvnRevision, leftSvnRevision, platforms
        );
        var task = GraphDiscoveryTask.graphEvaluationStarted(
                rightRevision, leftRevision, rightSvnRevision, leftSvnRevision, sandboxResponse.getId(), platforms,
                sandboxResponse.getStatus().name(), clock.instant()
        );
        db.currentOrTx(() -> db.graphDiscoveryTaskTable().save(task));
        log.info("State updated to {} for {}", task.getStatus(), task.getId());
        return task;
    }

    private SandboxTaskOutput sendTaskCreationRequestToSandbox(ArcRevision rightRevision,
                                                               ArcRevision leftRevision,
                                                               long rightSvnRevision,
                                                               long leftSvnRevision,
                                                               Set<GraphDiscoveryTask.Platform> platforms) {
        var sandboxTask = ChangesDetectorSandboxTaskBuilder.builder()
                .type(properties.getSandboxTaskType())
                .owner(properties.getSandboxTaskOwner())
                .priority(new SandboxTaskPriority(
                        SandboxTaskPriority.PriorityClass.SERVICE,
                        SandboxTaskPriority.PrioritySubclass.HIGH
                ))
                .rightRevision(rightRevision)
                .leftRevision(leftRevision)
                .rightSvnRevision(rightSvnRevision)
                .leftSvnRevision(leftSvnRevision)
                .platforms(platforms)
                .useDistbuildTestingCluster(properties.isUseDistbuildTestingCluster())
                .yaToken(properties.getSecretWithYaToolToken())
                .distbuildPool(properties.getDistBuildPool())
                .build()
                .asSandboxTask();
        log.info("Creating sandbox task for ({}, {}/{}, {})", rightRevision, leftSvnRevision, rightSvnRevision,
                platforms);
        var sandboxResponse = sandboxClient.createTask(sandboxTask);

        log.info("Created sandbox task (sandboxTaskId {}, status {}) for ({}, {}/{}, {})", sandboxResponse.getId(),
                sandboxResponse.getStatus(), rightRevision, leftSvnRevision, rightSvnRevision, platforms);
        return sandboxResponse;
    }

    private void startTasksInSandbox(List<GraphDiscoveryTask> tasks, boolean restart) {
        var sandboxTaskIdToTask = tasks.stream()
                .collect(Collectors.toMap(
                        GraphDiscoveryTask::getSandboxTaskId, Function.identity()
                ));
        var sandboxTaskIds = sandboxTaskIdToTask.keySet();
        log.info("Starting sandbox tasks {}", sandboxTaskIds);
        var sandboxTaskIdToStartResponse = sandboxClient
                .startTasks(sandboxTaskIds, DEFAULT_COMMENT_FOR_SANDBOX_TASK)
                .stream()
                .collect(Collectors.toMap(
                        BatchResult::getId, Function.identity()
                ));
        log.info("Started sandbox tasks {}: {}", sandboxTaskIds, sandboxTaskIdToStartResponse);

        var sandboxTaskIdToResponse = sandboxClient.getTasks(sandboxTaskIds, Set.of("id", "status"));
        db.currentOrTx(() -> {
            for (var response : sandboxTaskIdToResponse.values()) {
                db.graphDiscoveryTaskTable().findBySandboxTaskId(response.getId())
                        .filter(task ->
                                Optional.ofNullable(sandboxTaskIdToStartResponse.get(task.getSandboxTaskId()))
                                        .filter(it -> it.getStatus() != BatchStatus.ERROR)
                                        .isPresent()
                        )
                        .map(task -> {
                            var builder = task.toBuilder()
                                    .sandboxStatus(response.getStatus().name())
                                    .status(GRAPH_EVALUATION_STARTED);
                            if (restart) {
                                builder.attemptNumber(task.getAttemptNumber() + 1);
                            }
                            return builder.build();
                        })
                        .ifPresent(task -> {
                            db.graphDiscoveryTaskTable().save(task);
                            log.info("Sandbox status updated to {} for {}", task.getStatus(), task.getId());
                        });
            }
        });

        var fails = sandboxTaskIdToStartResponse.values().stream()
                .filter(it -> it.getStatus() == BatchStatus.ERROR)
                .toList();
        if (fails.size() > 0) {
            throw new GraphDiscoveryException("Failed to start some tasks: " + fails);
        }
    }

    public void updateStateOfRunningSandboxTasks() {
        runActionForTasksInStatus(GRAPH_EVALUATION_STARTED, tasks -> {
            if (!tasks.isEmpty()) {
                sandboxClient.getTasks(
                        tasks.stream().map(GraphDiscoveryTask::getSandboxTaskId).collect(Collectors.toSet()),
                        Set.of("id", "status", "execution"),
                        response -> db.currentOrTx(() -> {
                            var now = clock.instant();
                            for (var sandboxTaskOutput : response.values()) {
                                updateStateOfRunningSandboxTask(sandboxTaskOutput, now);
                            }
                        })
                );
            }
        });
    }

    public void runActionForTasksInStatus(GraphDiscoveryTask.Status status, Consumer<List<GraphDiscoveryTask>> action) {
        long lastMaxSandboxTaskId = -1;
        while (true) {
            long finalLastMaxSandboxTaskId = lastMaxSandboxTaskId;
            List<GraphDiscoveryTask> tasks = db.currentOrReadOnly(
                    () -> db.graphDiscoveryTaskTable().findByStatusAndSandboxTaskIdGreaterThen(
                            status, finalLastMaxSandboxTaskId, ListRequest.Builder.MAX_PAGE_SIZE
                    )
            );
            action.accept(tasks);
            if (tasks.size() < ListRequest.Builder.MAX_PAGE_SIZE) {
                break;
            }
            lastMaxSandboxTaskId = tasks.get(tasks.size() - 1).getSandboxTaskId();
        }
    }

    private void updateStateOfRunningSandboxTask(SandboxTaskOutput sandboxTaskOutput, Instant now) {
        var newStatus = sandboxTaskOutput.getStatusEnum();
        if (newStatus.isRunning()) {
            return;
        }

        var foundTask = db.graphDiscoveryTaskTable()
                .findBySandboxTaskId(sandboxTaskOutput.getId())
                .orElseThrow(() -> new GraphDiscoveryException(
                        "GraphDiscoveryTask not found with sandboxTaskId " + sandboxTaskOutput.getId()
                ));
        var updatedTask = foundTask.toBuilder()
                .status(
                        newStatus == SandboxTaskStatus.SUCCESS
                                ? GRAPH_EVALUATION_SUCCESS
                                : GRAPH_EVALUATION_FAILED
                )
                .sandboxStatus(newStatus.name())
                .nextAttemptAfter(
                        now.plus(
                                properties.getDelayBetweenSandboxTaskRestarts().toSeconds()
                                        * foundTask.getAttemptNumber(),
                                ChronoUnit.SECONDS
                        )
                )
                .graphEvaluationFinishedAt(
                        Optional.ofNullable(sandboxTaskOutput.getExecution())
                                .map(TaskExecution::getFinished)
                                .filter(it -> newStatus == SandboxTaskStatus.SUCCESS)
                                .orElse(null)
                )
                .build();
        db.graphDiscoveryTaskTable().save(updatedTask);
        log.info("State updated to {} for {}", updatedTask.getStatus(), updatedTask.getId());
    }

    public void updateStateOfFailedSandboxTasks() {
        runActionForTasksInStatus(GRAPH_EVALUATION_FAILED, tasks -> {
            if (!tasks.isEmpty()) {
                sandboxClient.getTasks(
                        tasks.stream().map(GraphDiscoveryTask::getSandboxTaskId).collect(Collectors.toSet()),
                        Set.of("id", "status", "execution"),
                        response -> db.currentOrTx(() -> {
                            response.values().forEach(this::updateStateOfFailedSandboxTask);
                        })
                );
            }
        });
    }

    private void updateStateOfFailedSandboxTask(SandboxTaskOutput sandboxTaskOutput) {
        var newStatus = sandboxTaskOutput.getStatusEnum();
        if (!newStatus.isRunning()) {
            return;
        }

        var updatedTask = db.graphDiscoveryTaskTable()
                .findBySandboxTaskId(sandboxTaskOutput.getId())
                .orElseThrow(() -> new GraphDiscoveryException(
                        "GraphDiscoveryTask not found with sandboxTaskId " + sandboxTaskOutput.getId()
                ))
                .toBuilder()
                .status(GRAPH_EVALUATION_STARTED)
                .sandboxStatus(newStatus.name())
                .graphEvaluationFinishedAt(null)
                .build();
        db.graphDiscoveryTaskTable().save(updatedTask);
        log.info("State updated to {} for {}", updatedTask.getStatus(), updatedTask.getId());
    }

    public AtomicLong getStartedTaskCount() {
        return startedTaskCount;
    }

    public AtomicLong getProcessedResultCount() {
        return processedResultCount;
    }

    private static List<Long> getSandboxTaskIds(List<GraphDiscoveryTask> tasks) {
        return tasks.stream().map(GraphDiscoveryTask::getSandboxTaskId).collect(Collectors.toList());
    }

    private static boolean shouldBeRecreated(GraphDiscoveryTask task, int attemptsNumberThreshold) {
        return shouldBeRecreatedCauseStatus(task)
                || shouldBeRecreatedCauseAttemptsNumber(task, attemptsNumberThreshold);
    }

    private static boolean shouldBeRecreatedCauseStatus(GraphDiscoveryTask task) {
        return SandboxTaskStatus.FAILURE.name().equals(task.getSandboxStatus());
    }

    private static boolean shouldBeRecreatedCauseAttemptsNumber(GraphDiscoveryTask task, int attemptsNumberThreshold) {
        // attempts number starts from 1
        return task.getAttemptNumber() > attemptsNumberThreshold * (task.getFailedSandboxTaskIds().size() + 1);
    }


    @Value(staticConstructor = "of")
    private static class Pair {
        GraphDiscoveryTask task;
        SandboxTaskOutput sandboxResponse;
    }

    @Value
    private static class RestartTaskContext {
        List<GraphDiscoveryTask> restartTask = new ArrayList<>();
        List<GraphDiscoveryTask> createNewTask = new ArrayList<>();

        RestartTaskContext(List<GraphDiscoveryTask> tasks, int attemptsNumberThreshold) {
            for (var task : tasks) {
                var running = SandboxTaskStatus.valueOf(task.getSandboxStatus()).isRunning();
                if (running) {
                    log.info("Skipping sandbox task {} with running status {}", task.getSandboxTaskId(),
                            task.getStatus());
                    continue;
                }
                if (shouldBeRecreated(task, attemptsNumberThreshold)) {
                    log.info("Sandbox task {} would be recreated: status {}, attempts {}, " +
                                    "failed sandbox tasks {}",
                            task.getSandboxTaskId(), task.getStatus(), task.getAttemptNumber(),
                            task.getFailedSandboxTaskIds().size()
                    );
                    createNewTask.add(task);
                } else {
                    log.info("Sandbox task {} would be restarted: status {}, attempts {}, failed sandbox tasks {}",
                            task.getSandboxTaskId(), task.getStatus(), task.getAttemptNumber(),
                            task.getFailedSandboxTaskIds().size()
                    );
                    restartTask.add(task);
                }
            }
        }

    }
}
