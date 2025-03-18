package ru.yandex.ci.client.ayamler;

import java.util.Set;
import java.util.stream.Collectors;

import io.grpc.inprocess.InProcessServerBuilder;
import io.grpc.stub.StreamObserver;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.RegisterExtension;

import ru.yandex.ci.ayamler.AYamlerServiceGrpc.AYamlerServiceImplBase;
import ru.yandex.ci.ayamler.Ayamler;
import ru.yandex.ci.ayamler.Ayamler.GetStrongModeBatchRequest;
import ru.yandex.ci.ayamler.Ayamler.GetStrongModeBatchResponse;
import ru.yandex.ci.ayamler.Ayamler.GetStrongModeRequest;
import ru.yandex.ci.ayamler.Ayamler.GetStrongModeResponse;
import ru.yandex.ci.common.grpc.GrpcCleanupExtension;
import ru.yandex.ci.common.grpc.GrpcClientPropertiesStub;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.AdditionalAnswers.delegatesTo;
import static org.mockito.Mockito.mock;

class AYamlerClientImplTest {

    private final AYamlerServiceImplBase aYamlerApiServiceMock = mock(
            AYamlerServiceImplBase.class, delegatesTo(new AYamlerServiceMock())
    );

    @RegisterExtension
    public GrpcCleanupExtension grpcCleanup = new GrpcCleanupExtension();
    private AYamlerClient aYamlerClient;

    @BeforeEach
    public void setUp() throws Exception {
        String serverName = InProcessServerBuilder.generateName();
        grpcCleanup.register(
                InProcessServerBuilder
                        .forName(serverName)
                        .directExecutor()
                        .addService(aYamlerApiServiceMock)
                        .build()
                        .start()
        );
        var properties = GrpcClientPropertiesStub.of(serverName, grpcCleanup);
        aYamlerClient = AYamlerClientImpl.create(properties);
    }

    @Test
    void getStrongModeBatch() throws Exception {
        var response = aYamlerClient.getStrongMode(
                        Set.of(
                                new StrongModeRequest("path1", "rev2", "check-author-1"),
                                new StrongModeRequest("path2", "rev3", "check-author-2")
                        )
                )
                .get();
        assertThat(response.getStrongModeList()).containsExactlyInAnyOrder(
                Ayamler.StrongMode.newBuilder()
                        .setPath("path1")
                        .setRevision("rev2")
                        .setStatus(Ayamler.StrongModeStatus.OFF)
                        .setLogin("check-author-1")
                        .build(),
                Ayamler.StrongMode.newBuilder()
                        .setPath("path2")
                        .setRevision("rev3")
                        .setStatus(Ayamler.StrongModeStatus.OFF)
                        .setLogin("check-author-2")
                        .build()
        );
    }


    private static class AYamlerServiceMock extends AYamlerServiceImplBase {
        @Override
        public void getStrongMode(GetStrongModeRequest request,
                                  StreamObserver<GetStrongModeResponse> responseObserver) {
            responseObserver.onNext(
                    GetStrongModeResponse.newBuilder()
                            .setStrongMode(Ayamler.StrongMode.newBuilder()
                                    .setPath(request.getPath())
                                    .setRevision(request.getRevision())
                                    .setStatus(Ayamler.StrongModeStatus.ON)
                                    .setAyaml(Ayamler.AYaml.newBuilder()
                                            .setValid(true)
                                            .setPath("path/a.yaml")
                                    )
                                    .setLogin(request.getLogin())
                            )
                            .build()
            );
            responseObserver.onCompleted();
        }

        @Override
        public void getStrongModeBatch(
                GetStrongModeBatchRequest batchRequest,
                StreamObserver<GetStrongModeBatchResponse> responseObserver
        ) {
            responseObserver.onNext(
                    GetStrongModeBatchResponse.newBuilder()
                            .addAllStrongMode(
                                    batchRequest.getRequestList()
                                            .stream()
                                            .map(request ->
                                                    Ayamler.StrongMode.newBuilder()
                                                            .setPath(request.getPath())
                                                            .setRevision(request.getRevision())
                                                            .setStatus(Ayamler.StrongModeStatus.OFF)
                                                            .setLogin(request.getLogin())
                                                            .build()
                                            )
                                            .collect(Collectors.toList())
                            )
                            .build()
            );
            responseObserver.onCompleted();
        }
    }

}
