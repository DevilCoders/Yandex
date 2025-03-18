package ru.yandex.ci.core.arc;

import java.util.concurrent.TimeUnit;
import java.util.function.Function;
import java.util.function.Supplier;

import io.grpc.ManagedChannel;
import io.grpc.ManagedChannelBuilder;
import io.grpc.stub.AbstractStub;

import ru.yandex.arc.api.BranchServiceGrpc;
import ru.yandex.arc.api.BranchServiceGrpc.BranchServiceImplBase;
import ru.yandex.arc.api.CommitServiceGrpc;
import ru.yandex.arc.api.CommitServiceGrpc.CommitServiceImplBase;
import ru.yandex.arc.api.DiffServiceGrpc;
import ru.yandex.arc.api.DiffServiceGrpc.DiffServiceImplBase;
import ru.yandex.arc.api.FileServiceGrpc;
import ru.yandex.arc.api.FileServiceGrpc.FileServiceImplBase;
import ru.yandex.arc.api.HistoryServiceGrpc;
import ru.yandex.arc.api.HistoryServiceGrpc.HistoryServiceImplBase;
import ru.yandex.ci.client.tvm.grpc.OAuthCallCredentials;
import ru.yandex.ci.common.grpc.GrpcClient;
import ru.yandex.ci.core.spring.clients.ArcClientTestConfig;
import ru.yandex.ci.core.test.canon.CanonizedGrpcServerBuilder;

/**
 * ArcService, который использует в качестве grpc сервисов - моки на локальных файлах.
 */
public class ArcServiceCanonImpl extends ArcServiceImpl {
    private static final ManagedChannel REAL_ARC_CHANNEL =
            ManagedChannelBuilder.forAddress("api.arc-vcs.yandex-team.ru", 6734).build();

    public ArcServiceCanonImpl(boolean canonize) {
        super(makeClient(canonize), null, false);
    }

    private static GrpcClient makeClient(boolean canonize) {
        try {
            var historyService = HistoryServiceGrpc.newBlockingStub(REAL_ARC_CHANNEL);
            var branchService = BranchServiceGrpc.newBlockingStub(REAL_ARC_CHANNEL);
            var fileService = FileServiceGrpc.newBlockingStub(REAL_ARC_CHANNEL);
            var commitService = CommitServiceGrpc.newBlockingStub(REAL_ARC_CHANNEL);
            var diffService = DiffServiceGrpc.newBlockingStub(REAL_ARC_CHANNEL);

            if (canonize) {
                OAuthCallCredentials credentials = new OAuthCallCredentials(getToken());
                historyService = historyService.withCallCredentials(credentials);
                branchService = branchService.withCallCredentials(credentials);
                fileService = fileService.withCallCredentials(credentials);
                commitService = commitService.withCallCredentials(credentials);
                diffService = diffService.withCallCredentials(credentials);
            }

            var channel = CanonizedGrpcServerBuilder.builder()
                    .withService(historyService, HistoryServiceImplBase.class)
                    .withService(branchService, BranchServiceImplBase.class)
                    .withService(fileService, FileServiceImplBase.class)
                    .withService(commitService, CommitServiceImplBase.class)
                    .withService(diffService, DiffServiceImplBase.class)
                    .doCanonize(canonize)
                    .buildAndStart();

            return new GrpcClient() {
                @Override
                public <T extends AbstractStub<T>> Supplier<T> buildStub(Function<ManagedChannel, T> constructor) {
                    var stub = constructor.apply(channel);
                    return () -> stub.withDeadlineAfter(40, TimeUnit.SECONDS);
                }

                @Override
                public void close() {
                    channel.shutdown();
                }
            };
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }

    private static String getToken() {
        String envKey = "ayamler.arcServiceChannel.token";
        String token = System.getenv(envKey);
        if (token == null) {
            System.err.println("Provide arc token via env var " + envKey +
                    " or turn off canonization at " + ArcClientTestConfig.class.getName());
            System.exit(1);
        }
        return token;
    }
}
