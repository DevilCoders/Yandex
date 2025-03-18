package ru.yandex.ci.storage.api.controllers;

import java.time.Instant;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.function.Predicate;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;

import com.google.common.base.Strings;
import io.grpc.Context;
import io.grpc.stub.StreamObserver;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.common.grpc.GrpcUtils;
import ru.yandex.ci.storage.api.StorageApi;
import ru.yandex.ci.storage.api.StorageApi.FindCheckByRevisionsRequest;
import ru.yandex.ci.storage.api.StorageApi.FindCheckByRevisionsResponse;
import ru.yandex.ci.storage.api.StorageApi.RegisterCheckIterationRequest;
import ru.yandex.ci.storage.api.StorageApi.RegisterCheckRequest;
import ru.yandex.ci.storage.api.StorageApi.RegisterCheckResponse;
import ru.yandex.ci.storage.api.StorageApi.RegisterTaskRequest;
import ru.yandex.ci.storage.api.StorageApiServiceGrpc;
import ru.yandex.ci.storage.api.StorageProxyApi;
import ru.yandex.ci.storage.api.cache.StorageApiCache;
import ru.yandex.ci.storage.api.check.ApiCheckService;
import ru.yandex.ci.storage.api.check.CheckComparer;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.CheckIteration.IterationType;
import ru.yandex.ci.storage.core.CheckOuterClass;
import ru.yandex.ci.storage.core.CheckTaskOuterClass;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.Common.CheckStatus;
import ru.yandex.ci.storage.core.check.CreateIterationParams;
import ru.yandex.ci.storage.core.db.constant.CheckStatusUtils;
import ru.yandex.ci.storage.core.db.constant.StorageLimits;
import ru.yandex.ci.storage.core.db.constant.TestenvUtils;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.check_task.CheckTaskEntity;
import ru.yandex.ci.storage.core.db.model.check_task.ExpectedTask;
import ru.yandex.ci.storage.core.db.model.suite_restart.SuiteRestartEntity;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffByHashEntity;
import ru.yandex.ci.storage.core.large.AutocheckTasksFactory;
import ru.yandex.ci.storage.core.logbroker.badge_events.BadgeEventsProducer;
import ru.yandex.ci.storage.core.logbroker.event_producer.StorageEventsProducer;
import ru.yandex.ci.storage.core.proto.CheckProtoMappers;

@Slf4j
@RequiredArgsConstructor
public class StorageApiController extends StorageApiServiceGrpc.StorageApiServiceImplBase {

    @Nonnull
    private final ApiCheckService checkService;

    @Nonnull
    private final StorageApiCache apiCache;

    @Nonnull
    private final StorageEventsProducer storageEventsProducer;

    @Nonnull
    private final BadgeEventsProducer badgeEventsProducer;

    @Nonnull
    private final AutocheckTasksFactory autocheckTasksFactory;

    @Nonnull
    private final CheckComparer checkComparer;

    @Nonnull
    private final StorageProxyApiController storageProxyApiController;

    @Override
    public void registerCheck(
            RegisterCheckRequest request,
            StreamObserver<RegisterCheckResponse> response
    ) {
        Context.current().fork().run(() -> registerCheckInternal(request, response));
    }

    @Override
    public void cancelIteration(
            StorageApi.CancelIterationRequest request,
            StreamObserver<CheckIteration.Iteration> response
    ) {
        Context.current().fork().run(() -> cancelIterationInternal(request, response));
    }

    @Override
    public void registerCheckIteration(
            RegisterCheckIterationRequest request,
            StreamObserver<CheckIteration.Iteration> response
    ) {
        Context.current().fork().run(() -> registerCheckIterationInternal(request, response));
    }

    @Override
    public void findChecksByRevisions(
            FindCheckByRevisionsRequest request,
            StreamObserver<FindCheckByRevisionsResponse> response
    ) {
        Context.current().fork().run(() -> findChecksByRevisionsInternal(request, response));
    }

    @Override
    public void registerTask(
            RegisterTaskRequest request, StreamObserver<CheckTaskOuterClass.CheckTask> response
    ) {
        Context.current().fork().run(() -> {
            registerTaskInternal(request, response);
        });
    }

    @Override
    public void cancelCheck(
            StorageApi.CancelCheckRequest request,
            StreamObserver<CheckOuterClass.Check> response
    ) {
        Context.current().fork().run(() -> cancelCheckInternal(request, response));
    }

    @Override
    public void setTestenvId(
            StorageApi.SetTestenvIdRequest request,
            StreamObserver<StorageApi.SetTestenvIdResponse> response
    ) {
        Context.current().fork().run(() -> setTestenvIdInternal(request, response));
    }

    private void setTestenvIdInternal(
            StorageApi.SetTestenvIdRequest request, StreamObserver<StorageApi.SetTestenvIdResponse> response
    ) {
        this.apiCache.modifyWithDbTx(
                cache -> {
                    var check = cache.checks().getFreshOrThrow(CheckEntity.Id.of(request.getCheckId()));
                    cache.checks().writeThrough(
                            check.toBuilder()
                                    .testenvId(request.getTestenvId())
                                    .build()
                    );
                }
        );

        response.onNext(StorageApi.SetTestenvIdResponse.newBuilder().build());
        response.onCompleted();
    }

    @Override
    public void getCheck(
            StorageApi.GetCheckRequest request,
            StreamObserver<StorageApi.GetCheckResponse> responseObserver
    ) {
        var check = this.apiCache.checks().getFreshOrThrow(CheckEntity.Id.of(request.getId()));
        responseObserver.onNext(
                StorageApi.GetCheckResponse.newBuilder()
                        .setCheck(CheckProtoMappers.toProtoCheck(check))
                        .build()
        );

        responseObserver.onCompleted();
    }

    @Override
    public void getIteration(
            CheckIteration.IterationId request,
            StreamObserver<CheckIteration.Iteration> responseObserver
    ) {
        var iteration = this.apiCache.iterations().getFreshOrThrow(CheckProtoMappers.toIterationId(request));
        responseObserver.onNext(CheckProtoMappers.toProtoIteration(iteration));
        responseObserver.onCompleted();
    }

    @Override
    public void getSuiteRestarts(
            StorageApi.GetSuiteRestartsRequest request,
            StreamObserver<StorageApi.GetSuiteRestartsResponse> responseObserver
    ) {
        var suiteRestarts = this.checkService.getSuiteRestarts(
                CheckProtoMappers.toIterationId(request.getIterationId())
        );

        responseObserver.onNext(
                StorageApi.GetSuiteRestartsResponse.newBuilder()
                        .addAllSuites(
                                suiteRestarts.stream()
                                        .map(this::convert)
                                        .collect(Collectors.toList())
                        )
                        .build()
        );

        responseObserver.onCompleted();
    }

    @Override
    public void allowTestenvFinish(
            StorageApi.AllowTestenvFinishRequest request,
            StreamObserver<StorageApi.AllowTestenvFinishResponse> responseObserver
    ) {
        var iterationId = CheckProtoMappers.toIterationId(request.getIterationId());
        var updatedIteration = this.apiCache.modifyWithDbTxAndGet(
                cache -> {
                    var iteration = cache.iterations().getFreshOrThrow(iterationId);
                    if (CheckStatusUtils.isCompleted(iteration.getStatus())) {
                        log.info("Testenv finish for completed iteration {}. Will skip", iterationId);
                        return iteration;
                    }

                    var task = new ExpectedTask(TestenvUtils.ALLOW_FINISH_TASK_PREFIX + iterationId.getNumber(), false);
                    if (iteration.getRegisteredExpectedTasks().contains(task)) {
                        log.info("Testenv finish already received for {}. Will skip", iterationId);
                        return iteration;
                    }

                    if (!iteration.getTasksType().equals(Common.CheckTaskType.CTT_TESTENV)) {
                        log.info("Testenv finish for not TE iteration {}. Will skip", iterationId);
                        return iteration;
                    }

                    log.info("Testenv finish received for {}", iterationId);

                    iteration = iteration.addExpectedTaskRegistered(task);
                    cache.iterations().writeThrough(iteration);

                    var metaIterationOptional = cache.iterations().getFresh(iterationId.toMetaId());
                    if (metaIterationOptional.isPresent()) {
                        var metaIteration = metaIterationOptional.get().addExpectedTaskRegistered(task);
                        log.info("Testenv finish will also be set for {}", metaIteration.getId());
                        cache.iterations().writeThrough(metaIteration);
                    }

                    return iteration;
                }
        );

        responseObserver.onNext(
                StorageApi.AllowTestenvFinishResponse.newBuilder()
                        .setIteration(CheckProtoMappers.toProtoIteration(updatedIteration))
                        .build()
        );

        responseObserver.onCompleted();
    }

    @Override
    public void getLargeTask(StorageApi.GetLargeTaskRequest request,
                             StreamObserver<StorageApi.GetLargeTaskResponse> responseObserver) {
        var response = autocheckTasksFactory.loadLargeTask(request);
        responseObserver.onNext(response);
        responseObserver.onCompleted();
    }

    private StorageApi.SuiteRestart convert(SuiteRestartEntity suite) {
        var id = suite.getId();
        return StorageApi.SuiteRestart.newBuilder()
                .setSuiteId(id.getSuiteId())
                .setIsRight(id.isRight())
                .setToolchain(id.getToolchain())
                .setPartition(id.getPartition())
                .setPath(suite.getPath())
                .setJobName(suite.getJobName())
                .build();
    }

    private void registerCheckInternal(
            RegisterCheckRequest request, StreamObserver<RegisterCheckResponse> responseObserver
    ) {
        log.info("New check requested");

        validateRegisterCheck(request);

        var requestedCheck = CheckProtoMappers.toCheck(request);
        log.info("Check registration requested {}", requestedCheck);
        var check = this.checkService.register(requestedCheck);
        if (check.getStatus() == CheckStatus.CREATED) {
            this.badgeEventsProducer.onCheckCreated(check);
        } else {
            log.info("Badge event skipped, check {} already in status {}", check.getId(), check.getStatus());
        }
        log.info("Check registered {}", check);

        responseObserver.onNext(
                RegisterCheckResponse.newBuilder()
                        .setCheck(CheckProtoMappers.toProtoCheck(check))
                        .build()
        );

        responseObserver.onCompleted();
    }

    private void validateRegisterCheck(RegisterCheckRequest request) {
        if (Strings.isNullOrEmpty(request.getRightRevision().getRevision())) {
            throw GrpcUtils.invalidArgumentException("Right revision is not set");
        }

        if (Strings.isNullOrEmpty(request.getLeftRevision().getRevision())) {
            throw GrpcUtils.invalidArgumentException("Left revision is not set");
        }

        if (Strings.isNullOrEmpty(request.getLeftRevision().getBranch())) {
            throw GrpcUtils.invalidArgumentException("Left branch is not set");
        }

        if (Strings.isNullOrEmpty(request.getRightRevision().getBranch())) {
            throw GrpcUtils.invalidArgumentException("Right branch is not set");
        }
    }

    private void registerCheckIterationInternal(
            RegisterCheckIterationRequest request,
            StreamObserver<CheckIteration.Iteration> response
    ) {
        log.info(
                "New check iteration for {}, testenv id: {}",
                request.getCheckId(), request.getInfo().getTestenvCheckId()
        );

        var checkId = CheckEntity.Id.of(request.getCheckId());
        var check = this.apiCache.checks().getFreshOrThrow(checkId);

        if (request.getNumber() > StorageLimits.ITERATIONS_LIMIT) {
            throw GrpcUtils.resourceExhausted(
                    "Check %s can not have more iterations of type %s".formatted(checkId, request.getCheckType())
            );
        }

        var iteration = apiCache.modifyWithDbTxAndGet(cache ->
                this.checkService.registerIterationInTx(
                        cache,
                        CheckIterationEntity.Id.of(
                                check.getId(),
                                IterationType.valueOf(request.getCheckType().name()),
                                request.getNumber()
                        ),
                        CreateIterationParams.builder()
                                .info(CheckProtoMappers.toIterationInfo(request.getInfo()))
                                .expectedTasks(
                                        request.getExpectedTasksList().stream()
                                                .map(task -> new ExpectedTask(task.getJobName(), task.getRight()))
                                                .collect(Collectors.toSet())
                                )
                                .autorun(request.getAutorun())
                                .tasksType(
                                        request.getTasksType().equals(Common.CheckTaskType.UNRECOGNIZED) ?
                                                Common.CheckTaskType.CTT_AUTOCHECK :
                                                request.getTasksType()
                                )
                                .build()
                )
        );

        var checkProto = CheckProtoMappers.toProtoCheck(check);
        var iterationProto = CheckProtoMappers.toProtoIteration(iteration);

        storageEventsProducer.onIterationRegistered(checkProto, iterationProto);

        response.onNext(CheckProtoMappers.toProtoIteration(iteration));
        response.onCompleted();
    }

    private void findChecksByRevisionsInternal(
            FindCheckByRevisionsRequest request,
            StreamObserver<FindCheckByRevisionsResponse> response
    ) {
        var checks = this.checkService.findChecksByRevisions(
                        request.getLeftRevision(), request.getRightRevision(), new HashSet<>(request.getTagsList())
                ).stream()
                .map(CheckProtoMappers::toProtoCheck)
                .collect(Collectors.toList());

        response.onNext(FindCheckByRevisionsResponse.newBuilder()
                .addAllChecks(checks)
                .build());
        response.onCompleted();
    }

    private void registerTaskInternal(
            RegisterTaskRequest request,
            StreamObserver<CheckTaskOuterClass.CheckTask> response
    ) {
        log.info(
                "New iteration task {} for {}/{}/{}",
                request.getTaskId(),
                request.getIterationId().getCheckId(),
                request.getIterationId().getCheckType(),
                request.getIterationId().getNumber()
        );

        if (Strings.isNullOrEmpty(request.getJobName())) {
            throw GrpcUtils.invalidArgumentException("Job name is blank");
        }

        if (request.getNumberOfPartitions() <= 0) {
            throw GrpcUtils.invalidArgumentException("Number of partitions is not set");
        }

        var checkId = CheckEntity.Id.of(request.getIterationId().getCheckId());
        var check = this.apiCache.checks().getFreshOrThrow(checkId);

        var iterationId = CheckProtoMappers.toIterationId(request.getIterationId());
        var iteration = apiCache.iterations().getFreshOrThrow(iterationId);

        if (iteration.getNumberOfTasks() >= StorageLimits.TASKS_LIMIT) {
            throw GrpcUtils.resourceExhausted(
                    "Iteration %s already has %d tasks".formatted(iterationId, iteration.getNumberOfTasks())
            );
        }

        var taskType = request.getType().equals(Common.CheckTaskType.UNRECOGNIZED) ?
                Common.CheckTaskType.CTT_AUTOCHECK : request.getType();

        var task = this.apiCache.modifyWithDbTxAndGet(cache -> checkService.registerTaskInTx(
                        cache,
                        CheckTaskEntity.builder()
                                .id(new CheckTaskEntity.Id(iteration.getId(), request.getTaskId()))
                                .numberOfPartitions(request.getNumberOfPartitions())
                                .right(request.getIsRightTask())
                                .status(CheckStatus.CREATED)
                                .jobName(request.getJobName())
                                .completedPartitions(Set.of())
                                .created(Instant.now())
                                .type(taskType)
                                .build()
                )
        );

        log.info("Task registered {}", task.getId());

        var checkProto = CheckProtoMappers.toProtoCheck(check);
        var iterationProto = CheckProtoMappers.toProtoIteration(iteration);
        var taskProto = CheckProtoMappers.toProtoCheckTask(task);

        // CI-3168 We should move events report to tms task, to make api faster and consistent on tx failure.
        storageEventsProducer.onTasksRegistered(checkProto, iterationProto, List.of(taskProto));
        response.onNext(taskProto);

        response.onCompleted();
    }

    private void cancelCheckInternal(
            StorageApi.CancelCheckRequest request,
            StreamObserver<CheckOuterClass.Check> response
    ) {
        var checkId = CheckEntity.Id.of(request.getId());
        var check = this.apiCache.checks().getFreshOrThrow(checkId);

        if (!CheckStatusUtils.isActive(check.getStatus())) {
            log.info("Check {} is not running, status: {}", check.getId(), check.getStatus());
            response.onNext(CheckProtoMappers.toProtoCheck(check));
            response.onCompleted();
            return;
        }

        log.info("Cancelling check {}", check.getId());

        check = checkService.onCancelRequested(check.getId());

        response.onNext(CheckProtoMappers.toProtoCheck(check));
        response.onCompleted();
    }

    private void cancelIterationInternal(
            StorageApi.CancelIterationRequest request,
            StreamObserver<CheckIteration.Iteration> response
    ) {
        var checkId = CheckEntity.Id.of(request.getId().getCheckId());
        this.apiCache.checks().getFreshOrThrow(checkId);

        var iterationId = CheckProtoMappers.toIterationId(request.getId());
        var iteration = apiCache.iterations().getFreshOrThrow(iterationId);

        if (!CheckStatusUtils.isActive(iteration.getStatus())) {
            log.info("Iteration {} is not running, status: {}", iteration.getId(), iteration.getStatus());
            response.onNext(CheckProtoMappers.toProtoIteration(iteration));
            response.onCompleted();
            return;
        }

        log.info("Cancelling iteration {}", iteration.getId());

        checkService.onCancelRequested(iteration.getId());

        storageEventsProducer.onCancelRequested(iteration.getId());

        response.onNext(CheckProtoMappers.toProtoIteration(iteration));
        response.onCompleted();
    }

    @Override
    public void compareLargeTasks(
            StorageApi.CompareLargeTasksRequest request,
            StreamObserver<StorageApi.CompareLargeTasksResponse> response
    ) {
        var result = this.checkComparer.compare(
                CheckProtoMappers.toLargeTaskEntityId(request.getLeft()),
                CheckProtoMappers.toLargeTaskEntityId(request.getRight()),
                getDiffTypeFilter(request)
        );

        response.onNext(
                StorageApi.CompareLargeTasksResponse.newBuilder()
                        .setCompareReady(result.isCompareReady())
                        .setCanceledLeft(result.isCanceledLeft())
                        .setCanceledRight(result.isCanceledRight())
                        .addAllDiffs(result.getDiffs().stream()
                                .map(StorageApiController::convert)
                                .toList())
                        .build()
        );
        response.onCompleted();
    }

    @Override
    public void sendMessages(
            StorageApi.SendMessagesRequest request,
            StreamObserver<StorageApi.SendMessagesResponse> responseObserver
    ) {
        var proxyRequest = StorageProxyApi.WriteMessagesRequest.newBuilder()
                .setCheckType(request.getCheckType())
                .addAllMessages(request.getMessagesList())
                .build();
        Context.current().fork().run(() -> {
            storageProxyApiController.writeMessages(proxyRequest);
            responseObserver.onNext(StorageApi.SendMessagesResponse.getDefaultInstance());
            responseObserver.onCompleted();
        });
    }

    private static StorageApi.TestDiff convert(TestDiffByHashEntity diff) {
        return StorageApi.TestDiff.newBuilder()
                .setTestId(CheckProtoMappers.toProtoTestId(diff.getId().getTestId()))
                .setPath(diff.getPath())
                .setName(diff.getName())
                .setSubtestName(diff.getSubtestName())
                .setLeft(diff.getLeft())
                .setRight(diff.getRight())
                .setResultType(diff.getResultType())
                .setIsMuted(diff.isMuted())
                .setIsStrongMode(diff.isStrongMode())
                .setDiffType(diff.getDiffType())
                .addAllTags(diff.getTags())
                .build();
    }

    private static Predicate<Common.TestDiffType> getDiffTypeFilter(StorageApi.CompareLargeTasksRequest request) {
        if (request.getFilterDiffTypesCount() == 0) {
            return diffType -> true;
        }
        var filter = Set.copyOf(request.getFilterDiffTypesList());
        return filter::contains;
    }
}
