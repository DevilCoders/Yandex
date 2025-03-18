package ru.yandex.ci.client.pciexpress;

import java.util.List;
import java.util.Map;

import io.grpc.inprocess.InProcessServerBuilder;
import io.grpc.stub.StreamObserver;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.RegisterExtension;

import ru.yandex.ci.common.grpc.GrpcCleanupExtension;
import ru.yandex.ci.common.grpc.GrpcClientPropertiesStub;
import ru.yandex.repo.pciexpress.proto.api.Api;
import ru.yandex.repo.pciexpress.proto.api.PCIExpressHandsGrpc.PCIExpressHandsImplBase;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.AdditionalAnswers.delegatesTo;
import static org.mockito.Mockito.mock;

public class PciExpressClientTest {

    private static final String FILE_NAME_PATTERN = "File_for_commitId_%s";

    private final PCIExpressHandsImplBase pciExpressHandsImplBaseMock = mock(
            PCIExpressHandsImplBase.class, delegatesTo(new PCIExpressHandsMock())
    );

    @RegisterExtension
    public GrpcCleanupExtension grpcCleanup = new GrpcCleanupExtension();
    private PciExpressClient pciExpressClient;

    @BeforeEach
    public void setUp() throws Exception {
        String channelName = InProcessServerBuilder.generateName();
        grpcCleanup.register(
                InProcessServerBuilder
                        .forName(channelName)
                        .directExecutor()
                        .addService(pciExpressHandsImplBaseMock)
                        .build()
                        .start()
        );

        var properties = GrpcClientPropertiesStub.of(channelName, grpcCleanup);
        pciExpressClient = PciExpressClient.create(properties);
    }

    @Test
    public void getProcessedCommitIds() {
        var commitIds = List.of("commitId1", "commitId2");
        var processedCommitIds = pciExpressClient.getPciDssCommitIdToStatus(commitIds);
        assertThat(processedCommitIds)
                .isEqualTo(Map.of(
                        "commitId1", Api.CommitStatus.PCI,
                        "commitId2", Api.CommitStatus.PCI
                ));
    }

    @Test
    public void getPathsByCommitId() {
        var commitId = "commitId1";
        var expected = List.of(FILE_NAME_PATTERN.formatted(commitId));
        var pathsByCommitId = pciExpressClient.getPathsByCommitId(commitId);
        assertThat(pathsByCommitId)
                .isEqualTo(expected);
    }

    private static class PCIExpressHandsMock extends PCIExpressHandsImplBase {
        @Override
        public void getCommitsFullInfo(Api.CommitsFullInfoRequest request,
                                       StreamObserver<Api.CommitsFullInfoResponse> responseObserver) {
            var commitFullInfos = request.getCommitsIdsList().stream()
                    .map(s -> {
                        var commitShortInfo = Api.CommitShortInfo.newBuilder()
                                .setCommitId(s)
                                .setCommitStatus(Api.CommitStatus.PCI)
                                .build();
                        return Api.CommitFullInfo.newBuilder()
                                .setCommitShortInfo(commitShortInfo)
                                .addFiles(FILE_NAME_PATTERN.formatted(s))
                                .build();
                    })
                    .toList();
            responseObserver.onNext(
                    Api.CommitsFullInfoResponse.newBuilder()
                            .addAllCommits(commitFullInfos)
                            .build()
            );
            responseObserver.onCompleted();
        }

        @Override
        public void getCommitPackages(Api.CommitPackageInfoRequest request,
                                      StreamObserver<Api.PackagesResponse> responseObserver) {
            Api.PackagesInfo packagesInfo = Api.PackagesInfo.newBuilder()
                    .addPackagesNames(FILE_NAME_PATTERN.formatted(request.getCommitId()))
                    .build();
            responseObserver.onNext(
                    Api.PackagesResponse.newBuilder()
                            .addComponentsPackages(packagesInfo)
                            .build()
            );
            responseObserver.onCompleted();
        }
    }
}
