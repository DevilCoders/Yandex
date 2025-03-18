package ru.yandex.ci.storage.tests.api.tester;

import java.util.List;
import java.util.concurrent.ExecutionException;
import java.util.function.Consumer;

import com.google.protobuf.Timestamp;
import io.grpc.internal.testing.StreamRecorder;

import ru.yandex.ci.storage.api.StorageApi;
import ru.yandex.ci.storage.api.controllers.StorageApiController;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.CheckOuterClass;
import ru.yandex.ci.storage.core.CheckTaskOuterClass;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.constant.Trunk;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_task.LargeTaskEntity;
import ru.yandex.ci.storage.core.proto.CheckProtoMappers;


public class StorageApiTester {
    private final StorageApiController apiController;

    public StorageApiTester(StorageApiController apiController) {
        this.apiController = apiController;
    }

    public StorageApi.RegisterCheckResponse registerCheck(
            Consumer<StorageApi.RegisterCheckRequest.Builder> updateCheck
    ) {
        var request = StorageApi.RegisterCheckRequest.newBuilder()
                .setLeftRevision(
                        Common.OrderedRevision.newBuilder()
                                .setBranch(Trunk.name())
                                .setRevision("r1")
                                .setRevisionNumber(1)
                                .setTimestamp(Timestamp.newBuilder().build())
                                .build()
                )
                .setRightRevision(
                        Common.OrderedRevision.newBuilder()
                                .setBranch("pr:42")
                                .setRevision("0396111a6031d8c0e0641e41a8627d6434973e13")
                                .setRevisionNumber(0)
                                .setTimestamp(Timestamp.newBuilder().build())
                                .build()
                )
                .setInfo(CheckOuterClass.CheckInfo.newBuilder().build())
                .setOwner("firov")
                .setTestRestartsAllowed(true)
                .setReportStatusToArcanum(true);
        updateCheck.accept(request);

        var response = StreamRecorder.<StorageApi.RegisterCheckResponse>create();
        apiController.registerCheck(request.build(), response);

        try {
            return response.firstValue().get();
        } catch (Exception e) {
            throw new RuntimeException("", e);
        }
    }

    public CheckIteration.Iteration registerIteration(
            CheckEntity check, CheckIteration.IterationType iterationType, int number,
            Consumer<StorageApi.RegisterCheckIterationRequest.Builder> updateIteration
    ) {
        var request = StorageApi.RegisterCheckIterationRequest.newBuilder()
                .setCheckId(check.getId().getId().toString())
                .setCheckType(iterationType)
                .setNumber(number);
        updateIteration.accept(request);

        var response = StreamRecorder.<CheckIteration.Iteration>create();
        apiController.registerCheckIteration(request.build(), response);

        try {
            return response.firstValue().get();
        } catch (Exception e) {
            throw new RuntimeException("", e);
        }
    }

    public CheckTaskOuterClass.CheckTask registerTask(
            CheckIteration.IterationId iterationId, String id, int numberOfPartitions,
            boolean right, Common.CheckTaskType taskType
    ) {
        var response = StreamRecorder.<CheckTaskOuterClass.CheckTask>create();
        apiController.registerTask(
                StorageApi.RegisterTaskRequest.newBuilder()
                        .setIsRightTask(right)
                        .setIterationId(iterationId)
                        .setTaskId(id)
                        .setJobName(id)
                        .setNumberOfPartitions(numberOfPartitions)
                        .setType(taskType)
                        .build(),
                response
        );
        try {
            return response.firstValue().get();
        } catch (Exception e) {
            throw new RuntimeException("", e);
        }
    }

    public CheckIteration.Iteration allowTestenvFinish(CheckIteration.IterationId id) {
        var response = StreamRecorder.<StorageApi.AllowTestenvFinishResponse>create();
        this.apiController.allowTestenvFinish(
                StorageApi.AllowTestenvFinishRequest.newBuilder()
                        .setIterationId(id)
                        .build(),
                response
        );

        try {
            return response.firstValue().get().getIteration();
        } catch (InterruptedException | ExecutionException e) {
            throw new RuntimeException("Fail", e);
        }
    }

    public CheckIteration.Iteration cancel(CheckIteration.IterationId id) {
        var response = StreamRecorder.<CheckIteration.Iteration>create();
        apiController.cancelIteration(
                StorageApi.CancelIterationRequest.newBuilder()
                        .setId(id)
                        .build(),
                response
        );
        try {
            return response.firstValue().get();
        } catch (Exception e) {
            throw new RuntimeException("", e);
        }
    }

    public StorageApi.CompareLargeTasksResponse compareChecks(
            LargeTaskEntity.Id left,
            LargeTaskEntity.Id right,
            Common.TestDiffType... diffTypes
    ) {
        var response = StreamRecorder.<StorageApi.CompareLargeTasksResponse>create();
        apiController.compareLargeTasks(
                StorageApi.CompareLargeTasksRequest.newBuilder()
                        .setLeft(CheckProtoMappers.toProtoLargeTaskEntityId(left))
                        .setRight(CheckProtoMappers.toProtoLargeTaskEntityId(right))
                        .addAllFilterDiffTypes(List.of(diffTypes))
                        .build(),
                response
        );

        try {
            return response.firstValue().get();
        } catch (Exception e) {
            throw new RuntimeException("", e);
        }
    }
}
