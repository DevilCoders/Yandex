package ru.yandex.ci.client.storage;

import java.util.List;
import java.util.function.Supplier;

import com.google.common.base.Preconditions;

import ru.yandex.ci.common.grpc.GrpcClient;
import ru.yandex.ci.common.grpc.GrpcClientImpl;
import ru.yandex.ci.common.grpc.GrpcClientProperties;
import ru.yandex.ci.storage.api.StorageApi;
import ru.yandex.ci.storage.api.StorageApi.FindCheckByRevisionsRequest;
import ru.yandex.ci.storage.api.StorageApi.FindCheckByRevisionsResponse;
import ru.yandex.ci.storage.api.StorageApiServiceGrpc;
import ru.yandex.ci.storage.api.StorageApiServiceGrpc.StorageApiServiceBlockingStub;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.CheckIteration.Iteration;
import ru.yandex.ci.storage.core.CheckIteration.IterationId;
import ru.yandex.ci.storage.core.CheckIteration.IterationType;
import ru.yandex.ci.storage.core.CheckOuterClass;
import ru.yandex.ci.storage.core.CheckTaskOuterClass.CheckTask;
import ru.yandex.ci.storage.core.Common;

public class StorageApiClient implements AutoCloseable {

    private final GrpcClient grpcClient;
    private final Supplier<StorageApiServiceBlockingStub> stub;

    private StorageApiClient(GrpcClientProperties storageGrpcClientProperties) {
        this.grpcClient = GrpcClientImpl.builder(storageGrpcClientProperties, getClass())
                .excludeLoggingFullResponse(StorageApiServiceGrpc.getCompareLargeTasksMethod())
                .build();
        this.stub = grpcClient.buildStub(StorageApiServiceGrpc::newBlockingStub);
    }

    public static StorageApiClient create(GrpcClientProperties storageGrpcClientProperties) {
        return new StorageApiClient(storageGrpcClientProperties);
    }

    @Override
    public void close() throws Exception {
        grpcClient.close();
    }

    public CheckOuterClass.Check registerCheck(StorageApi.RegisterCheckRequest request) {
        return stub.get().registerCheck(request).getCheck();
    }

    public CheckOuterClass.Check getCheck(long id) {
        return stub.get().getCheck(StorageApi.GetCheckRequest.newBuilder().setId(id).build()).getCheck();
    }

    public CheckIteration.Iteration getIteration(IterationId iterationId) {
        return stub.get().getIteration(iterationId);
    }

    public List<StorageApi.SuiteRestart> getSuiteRestarts(IterationId iterationId) {
        return stub.get()
                .getSuiteRestarts(StorageApi.GetSuiteRestartsRequest.newBuilder().setIterationId(iterationId).build())
                .getSuitesList();
    }

    public Iteration registerCheckIteration(
            String checkId,
            IterationType iterationType,
            int number,
            List<CheckIteration.ExpectedTask> expectedTasks,
            CheckIteration.IterationInfo iterationInfo
    ) {

        return stub.get().registerCheckIteration(
                StorageApi.RegisterCheckIterationRequest
                        .newBuilder()
                        .setCheckId(checkId)
                        .setCheckType(iterationType)
                        .setNumber(number)
                        .addAllExpectedTasks(expectedTasks)
                        .setInfo(iterationInfo)
                        .build()
        );
    }

    public CheckTask registerTask(
            IterationId iterationId,
            String taskId,
            int numberOfPartitions,
            boolean isRightTask,
            String jobName,
            Common.CheckTaskType checkTaskType
    ) {
        return stub.get().registerTask(
                StorageApi.RegisterTaskRequest
                        .newBuilder()
                        .setIterationId(iterationId)
                        .setTaskId(taskId)
                        .setNumberOfPartitions(numberOfPartitions)
                        .setIsRightTask(isRightTask)
                        .setJobName(jobName)
                        .setType(checkTaskType)
                        .build()
        );
    }

    public StorageApi.SetTestenvIdResponse setTestenvId(String checkId, String testenvId) {
        return stub.get().setTestenvId(
                StorageApi.SetTestenvIdRequest
                        .newBuilder()
                        .setCheckId(checkId)
                        .setTestenvId(testenvId)
                        .build()
        );
    }

    public FindCheckByRevisionsResponse findChecksByRevisionsAndTags(FindCheckByRevisionsRequest request) {
        Preconditions.checkArgument(request.getTagsCount() != 0, "request.tags can not be empty");
        return stub.get().findChecksByRevisions(request);
    }

    public CheckOuterClass.Check cancelCheck(String checkId) {
        return stub.get().cancelCheck(StorageApi.CancelCheckRequest.newBuilder()
                .setId(checkId)
                .build());
    }

    public StorageApi.GetLargeTaskResponse getLargeTask(StorageApi.GetLargeTaskRequest request) {
        return stub.get().getLargeTask(request);
    }

    public StorageApi.CompareLargeTasksResponse compareLargeTasks(StorageApi.CompareLargeTasksRequest request) {
        return stub.get().compareLargeTasks(request);
    }

    public StorageApi.SendMessagesResponse sendMessages(StorageApi.SendMessagesRequest request) {
        return stub.get().sendMessages(request);
    }

}
