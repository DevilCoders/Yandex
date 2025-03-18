package ru.yandex.ci.client.storage;

import java.util.List;

import io.grpc.inprocess.InProcessServerBuilder;
import io.grpc.stub.StreamObserver;
import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.RegisterExtension;
import org.mockito.AdditionalAnswers;
import org.mockito.Mockito;

import ru.yandex.ci.common.grpc.GrpcCleanupExtension;
import ru.yandex.ci.common.grpc.GrpcClientPropertiesStub;
import ru.yandex.ci.storage.api.StorageApi;
import ru.yandex.ci.storage.api.StorageApi.CancelCheckRequest;
import ru.yandex.ci.storage.api.StorageApi.FindCheckByRevisionsRequest;
import ru.yandex.ci.storage.api.StorageApi.FindCheckByRevisionsResponse;
import ru.yandex.ci.storage.api.StorageApiServiceGrpc;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.CheckOuterClass;
import ru.yandex.ci.storage.core.CheckTaskOuterClass;
import ru.yandex.ci.storage.core.Common;

class StorageApiClientTest {

    private static final Common.OrderedRevision TRUNK_R1 = Common.OrderedRevision.newBuilder()
            .setRevision("r1")
            .setBranch("trunk")
            .setRevisionNumber(91)
            .build();
    private static final Common.OrderedRevision TRUNK_R0 = Common.OrderedRevision.newBuilder()
            .setRevision("r0")
            .setBranch("trunk")
            .setRevisionNumber(90)
            .build();

    private final StorageApiServiceGrpc.StorageApiServiceImplBase storageApiServiceMock = Mockito.mock(
            StorageApiServiceGrpc.StorageApiServiceImplBase.class,
            AdditionalAnswers.delegatesTo(new StorageApiServiceGrpc.StorageApiServiceImplBase() {
                @Override
                public void registerCheck(StorageApi.RegisterCheckRequest request,
                                          StreamObserver<StorageApi.RegisterCheckResponse> responseObserver) {
                    responseObserver.onNext(StorageApi.RegisterCheckResponse
                            .newBuilder()
                            .setCheck(CheckOuterClass.Check
                                    .newBuilder()
                                    .setDiffSetId(request.getDiffSetId())
                                    .setOwner(request.getOwner())
                                    .addAllTags(request.getTagsList())
                                    .setInfo(request.getInfo())
                                    .build()
                            )
                            .build());
                    responseObserver.onCompleted();
                }

                @Override
                public void registerCheckIteration(StorageApi.RegisterCheckIterationRequest request,
                                                   StreamObserver<CheckIteration.Iteration> responseObserver) {
                    responseObserver.onNext(CheckIteration.Iteration
                            .newBuilder()
                            .setId(CheckIteration.IterationId.newBuilder().setCheckId(request.getCheckId()).build())
                            .build());
                    responseObserver.onCompleted();
                }

                @Override
                public void registerTask(StorageApi.RegisterTaskRequest request,
                                         StreamObserver<CheckTaskOuterClass.CheckTask> responseObserver) {
                    responseObserver.onNext(CheckTaskOuterClass.CheckTask
                            .newBuilder()
                            .setId(CheckTaskOuterClass.FullTaskId
                                    .newBuilder()
                                    .setTaskId(request.getTaskId())
                                    .build())
                            .build());
                    responseObserver.onCompleted();
                }

                @Override
                public void findChecksByRevisions(FindCheckByRevisionsRequest request,
                                                  StreamObserver<FindCheckByRevisionsResponse> responseObserver) {
                    responseObserver.onNext(FindCheckByRevisionsResponse.newBuilder()
                            .addAllChecks(List.of(
                                    CheckOuterClass.Check
                                            .newBuilder()
                                            .setDiffSetId(100L)
                                            .setOwner("owner")
                                            .addAllTags(request.getTagsList())
                                            .build()
                            ))
                            .build()
                    );
                    responseObserver.onCompleted();
                }

                @Override
                public void cancelCheck(CancelCheckRequest request,
                                        StreamObserver<CheckOuterClass.Check> responseObserver) {
                    responseObserver.onNext(CheckOuterClass.Check
                            .newBuilder()
                            .setDiffSetId(100L)
                            .setOwner("owner")
                            .build());
                    responseObserver.onCompleted();
                }
            }));

    @RegisterExtension
    public GrpcCleanupExtension grpcCleanup = new GrpcCleanupExtension();
    private StorageApiClient storageApiClient;

    @BeforeEach
    public void setUp() throws Exception {
        String serverName = InProcessServerBuilder.generateName();
        grpcCleanup.register(InProcessServerBuilder
                .forName(serverName)
                .directExecutor()
                .addService(storageApiServiceMock)
                .build()
                .start());

        var properties = GrpcClientPropertiesStub.of(serverName, grpcCleanup);
        storageApiClient = StorageApiClient.create(properties);
    }

    @Test
    public void testCreateCheck() {
        var autoStart = CheckOuterClass.LargeAutostart.newBuilder()
                .setTarget("a/b/c")
                .addAllToolchains(List.of("tool-a", "tool-b"))
                .build();
        var check = storageApiClient.registerCheck(
                StorageApi.RegisterCheckRequest.newBuilder()
                        .setLeftRevision(TRUNK_R0)
                        .setRightRevision(TRUNK_R1)
                        .setOwner("owner")
                        .addAllTags(List.of("tags"))
                        .setInfo(
                                CheckOuterClass.CheckInfo.newBuilder()
                                        .addLargeAutostart(autoStart)
                                        .build()
                        )
                        .build()
        );
        Assertions.assertEquals("owner", check.getOwner());
        Assertions.assertEquals(List.of("tags"), check.getTagsList());
        Assertions.assertEquals(List.of(autoStart), check.getInfo().getLargeAutostartList());
    }

    @Test
    public void testCreateCheckIteration() {
        var checkIteration = storageApiClient.registerCheckIteration(
                "ac1",
                CheckIteration.IterationType.FULL,
                1,
                List.of(),
                CheckIteration.IterationInfo.getDefaultInstance()
        );
        Assertions.assertEquals("ac1", checkIteration.getId().getCheckId());
    }

    @Test
    public void testCreateTask() {
        var checkTask = storageApiClient.registerTask(
                CheckIteration.IterationId.newBuilder().build(),
                "123",
                6,
                true,
                "BUILD_TRUNK_META_LINUX_DISTBUILD",
                Common.CheckTaskType.CTT_AUTOCHECK);
        Assertions.assertEquals("123", checkTask.getId().getTaskId());
    }

    @Test
    public void findChecksByRevisionsAndTags() {
        var response = storageApiClient.findChecksByRevisionsAndTags(
                FindCheckByRevisionsRequest.newBuilder()
                        .setLeftRevision("left")
                        .setRightRevision("right")
                        .addAllTags(List.of("A", "B", "C"))
                        .build()
        );
        var expectedResponse = FindCheckByRevisionsResponse.newBuilder()
                .addAllChecks(List.of(
                        CheckOuterClass.Check
                                .newBuilder()
                                .setDiffSetId(100L)
                                .setOwner("owner")
                                .addAllTags(List.of("A", "B", "C"))
                                .build()
                ))
                .build();
        Assertions.assertEquals(expectedResponse, response);
    }

    @Test
    public void cancelCheck() {
        var response = storageApiClient.cancelCheck("check-id");
        var expectedResponse = CheckOuterClass.Check
                .newBuilder()
                .setDiffSetId(100L)
                .setOwner("owner")
                .build();
        Assertions.assertEquals(expectedResponse, response);
    }
}
