package yandex.cloud.team.integration.abc;

import java.time.Duration;
import java.util.UUID;
import java.util.function.Function;

import io.grpc.ManagedChannel;
import io.grpc.ManagedChannelBuilder;
import io.grpc.inprocess.InProcessChannelBuilder;
import lombok.Getter;
import yandex.cloud.grpc.GrpcHeaders;
import yandex.cloud.grpc.HeaderAttachingClientInterceptor;
import yandex.cloud.operation.client.test.TestOperationClient;
import yandex.cloud.priv.operation.PO;
import yandex.cloud.priv.team.integration.v1.AbcServiceGrpc;
import yandex.cloud.priv.team.integration.v1.OperationServiceGrpc;
import yandex.cloud.priv.team.integration.v1.POS;
import yandex.cloud.priv.team.integration.v1.PTIAS;

@Getter
public class TestAbcIntegrationClient {

    private static final Duration MAX_OPERATION_DURATION = Duration.ofMinutes(5);

    private final AbcServiceGrpc.AbcServiceBlockingStub abcService;
    private final TestOperationClient abcServiceOps;


    public TestAbcIntegrationClient(ManagedChannelBuilder<?> channelBuilder) {
        ManagedChannel channel = channelBuilder.build();
        abcService = AbcServiceGrpc.newBlockingStub(channel);

        var abcOperationService = OperationServiceGrpc.newBlockingStub(channel);
        Function<PO.Operation, PO.Operation> updater = op -> abcOperationService
                .withInterceptors(new HeaderAttachingClientInterceptor(GrpcHeaders.AUTHORIZATION, "Bearer ..."))
                .get(POS.GetOperationRequest.newBuilder()
                        .setOperationId(op.getId())
                        .build()
                );
        abcServiceOps = new TestOperationClient(updater, MAX_OPERATION_DURATION);
    }

    public static TestAbcIntegrationClient inProcess(String serverName) {
        return new TestAbcIntegrationClient(
                InProcessChannelBuilder.forName(serverName).directExecutor()
        );
    }

    public PTIAS.ResolveResponse resolveByCloud(String cloudId) {
        return resolve(PTIAS.ResolveRequest.newBuilder()
                .setCloudId(cloudId)
                .build()
        );
    }

    public PTIAS.ResolveResponse resolveBySlug(String abcSlug) {
        return resolve(PTIAS.ResolveRequest.newBuilder()
                .setAbcSlug(abcSlug)
                .build()
        );
    }

    public PTIAS.ResolveResponse resolveById(long abcId) {
        return resolve(PTIAS.ResolveRequest.newBuilder()
                .setAbcId(abcId)
                .build()
        );
    }

    public PO.Operation createBySlug(String abcSlug, String idempotencyKey) {
        if (idempotencyKey == null) {
            idempotencyKey = UUID.randomUUID().toString();
        }
        return abcService
                .withInterceptors(
                        new HeaderAttachingClientInterceptor(GrpcHeaders.IDEMPOTENCY_KEY, idempotencyKey),
                        new HeaderAttachingClientInterceptor(GrpcHeaders.AUTHORIZATION, "Bearer ...")
                )
                .create(PTIAS.CreateCloudRequest.newBuilder()
                        .setAbcSlug(abcSlug)
                        .build()
                );
    }

    public PO.Operation createById(long abcId, String idempotencyKey) {
        if (idempotencyKey == null) {
            idempotencyKey = UUID.randomUUID().toString();
        }
        return abcService
                .withInterceptors(
                        new HeaderAttachingClientInterceptor(GrpcHeaders.IDEMPOTENCY_KEY, idempotencyKey),
                        new HeaderAttachingClientInterceptor(GrpcHeaders.AUTHORIZATION, "Bearer ...")
                )
                .create(PTIAS.CreateCloudRequest.newBuilder()
                        .setAbcId(abcId)
                        .build()
                );
    }

    public PO.Operation createByAbcFolderId(String abcFolderId, String idempotencyKey) {
        if (idempotencyKey == null) {
            idempotencyKey = UUID.randomUUID().toString();
        }
        return abcService
                .withInterceptors(
                        new HeaderAttachingClientInterceptor(GrpcHeaders.IDEMPOTENCY_KEY, idempotencyKey),
                        new HeaderAttachingClientInterceptor(GrpcHeaders.AUTHORIZATION, "Bearer ...")
                )
                .create(PTIAS.CreateCloudRequest.newBuilder()
                        .setAbcFolderId(abcFolderId)
                        .build()
                );
    }

    public PTIAS.ResolveResponse resolve(PTIAS.ResolveRequest request) {
        return abcService
                .withInterceptors(
                        new HeaderAttachingClientInterceptor(GrpcHeaders.AUTHORIZATION, "Bearer ...")
                )
                .resolve(request);
    }

}
