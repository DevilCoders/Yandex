package yandex.cloud.team.integration.abc;

import io.grpc.stub.StreamObserver;
import org.jetbrains.annotations.NotNull;
import yandex.cloud.grpc.BaseOperationConverter;
import yandex.cloud.iam.grpc.GrpcCallHandler;
import yandex.cloud.model.operation.Operation;
import yandex.cloud.priv.operation.PO;
import yandex.cloud.priv.team.integration.v1.AbcServiceGrpc;
import yandex.cloud.priv.team.integration.v1.PTIAS;
import yandex.cloud.ti.abc.AbcServiceCloud;

public class AbcServiceGrpcImpl extends AbcServiceGrpc.AbcServiceImplBase {

    private final @NotNull CreateCloudServiceFacade createCloudServiceFacade;
    private final @NotNull ResolveServiceFacade resolveServiceFacade;
    private final @NotNull BaseOperationConverter operationConverter;
    private final @NotNull GrpcCallHandler callHandler;


    public AbcServiceGrpcImpl(
            @NotNull CreateCloudServiceFacade createCloudServiceFacade,
            @NotNull ResolveServiceFacade resolveServiceFacade,
            @NotNull BaseOperationConverter operationConverter,
            @NotNull GrpcCallHandler callHandler
    ) {
        this.createCloudServiceFacade = createCloudServiceFacade;
        this.resolveServiceFacade = resolveServiceFacade;
        this.operationConverter = operationConverter;
        this.callHandler = callHandler;
    }


    @Override
    public void create(
            @NotNull PTIAS.CreateCloudRequest request,
            @NotNull StreamObserver<PO.Operation> responseObserver
    ) {
        callHandler.invoke(responseObserver, () -> operationConverter.operation(create(request)));
    }

    @Override
    public void resolve(
            @NotNull PTIAS.ResolveRequest request,
            @NotNull StreamObserver<PTIAS.ResolveResponse> responseObserver
    ) {
        callHandler.invoke(responseObserver, () -> {
            AbcServiceCloud abcServiceCloud = resolve(request);
            return PTIAS.ResolveResponse.newBuilder()
                    .setAbcId(abcServiceCloud.abcServiceId())
                    .setAbcSlug(abcServiceCloud.abcServiceSlug())
                    .setAbcFolderId(abcServiceCloud.abcdFolderId())
                    .setCloudId(abcServiceCloud.cloudId())
                    .setDefaultFolderId(abcServiceCloud.defaultFolderId())
                    .build();
        });
    }

    private @NotNull AbcServiceCloud resolve(
            @NotNull PTIAS.ResolveRequest request
    ) {
        return switch (request.getAbcCase()) {
            case ABC_ID -> resolveServiceFacade.resolveAbcServiceCloudByAbcServiceId(request.getAbcId());
            case ABC_SLUG -> resolveServiceFacade.resolveAbcServiceCloudByAbcServiceSlug(request.getAbcSlug());
            case ABC_FOLDER_ID -> resolveServiceFacade.resolveAbcServiceCloudByAbcdFolderId(request.getAbcFolderId());
            case CLOUD_ID -> resolveServiceFacade.resolveAbcServiceCloudByCloudId(request.getCloudId());
            default -> throw InvalidResolveRequestException.empty();
        };
    }

    private @NotNull Operation create(
            @NotNull PTIAS.CreateCloudRequest request
    ) {
        return switch (request.getAbcCase()) {
            case ABC_ID -> createCloudServiceFacade.createByAbcServiceId(request.getAbcId());
            case ABC_SLUG -> createCloudServiceFacade.createByAbcServiceSlug(request.getAbcSlug());
            case ABC_FOLDER_ID -> createCloudServiceFacade.createByAbcdFolderId(request.getAbcFolderId());
            default -> throw InvalidCreateRequestException.empty();
        };
    }

}
