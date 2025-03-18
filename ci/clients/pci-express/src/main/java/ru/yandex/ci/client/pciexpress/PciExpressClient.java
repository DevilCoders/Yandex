package ru.yandex.ci.client.pciexpress;

import java.util.List;
import java.util.Map;
import java.util.function.Supplier;
import java.util.stream.Collectors;

import ru.yandex.ci.common.grpc.GrpcClient;
import ru.yandex.ci.common.grpc.GrpcClientImpl;
import ru.yandex.ci.common.grpc.GrpcClientProperties;
import ru.yandex.repo.pciexpress.proto.api.Api;
import ru.yandex.repo.pciexpress.proto.api.PCIExpressHandsGrpc;

public class PciExpressClient implements AutoCloseable {

    private final GrpcClient grpcClient;
    private final Supplier<PCIExpressHandsGrpc.PCIExpressHandsBlockingStub> pciExpressService;

    private PciExpressClient(GrpcClientProperties grpcClientProperties) {
        this.grpcClient = GrpcClientImpl.builder(grpcClientProperties, getClass()).build();
        pciExpressService = grpcClient.buildStub(PCIExpressHandsGrpc::newBlockingStub);
    }

    public static PciExpressClient create(GrpcClientProperties grpcClientProperties) {
        return new PciExpressClient(grpcClientProperties);
    }

    public Map<String, Api.CommitStatus> getPciDssCommitIdToStatus(Iterable<String> commitIds) {
        var request = Api.CommitsFullInfoRequest.newBuilder()
                .addAllCommitsIds(commitIds)
                .build();
        var response = pciExpressService.get().getCommitsFullInfo(request);
        return response.getCommitsList().stream()
                .map(Api.CommitFullInfo::getCommitShortInfo)
                .collect(Collectors.toMap(
                        Api.CommitShortInfo::getCommitId,
                        Api.CommitShortInfo::getCommitStatus
                ));
    }

    public List<String> getPathsByCommitId(String commitId) {
        var request = Api.CommitPackageInfoRequest.newBuilder()
                .setCommitId(commitId)
                .addStatuses(Api.CommitStatus.PCI)
                .build();
        var response = pciExpressService.get().getCommitPackages(request);
        return response.getComponentsPackagesList().stream()
                .flatMap(it -> it.getPackagesNamesList().stream())
                .toList();
    }

    @Override
    public void close() throws Exception {
        grpcClient.close();
    }
}
