package yandex.cloud.team.integration.abc;

import java.util.function.Consumer;
import java.util.function.Supplier;

import io.grpc.stub.StreamObserver;
import org.jetbrains.annotations.NotNull;
import yandex.cloud.grpc.BaseOperationConverter;
import yandex.cloud.iam.grpc.GrpcCallHandler;
import yandex.cloud.model.operation.OperationData;
import yandex.cloud.priv.operation.PO;
import yandex.cloud.priv.team.integration.v1.OperationServiceGrpc;
import yandex.cloud.priv.team.integration.v1.POS;

public class OperationServiceGrpcImpl extends OperationServiceGrpc.OperationServiceImplBase {

    private final @NotNull GrpcCallHandler callHandler;
    private final @NotNull BaseOperationConverter operationConverter;
    private final @NotNull OperationServiceFacade operationServiceFacade;


    public OperationServiceGrpcImpl(
            @NotNull GrpcCallHandler callHandler,
            @NotNull BaseOperationConverter operationConverter,
            @NotNull OperationServiceFacade operationServiceFacade
    ) {
        this.callHandler = callHandler;
        this.operationConverter = operationConverter;
        this.operationServiceFacade = operationServiceFacade;
    }


    @Override
    public void get(
            @NotNull POS.GetOperationRequest request,
            @NotNull StreamObserver<PO.Operation> responseObserver
    ) {
        operationCall(responseObserver).accept(
                () -> operationServiceFacade.getOperation(request.getOperationId())
        );
    }

    private @NotNull Consumer<Supplier<OperationData>> operationCall(
            @NotNull StreamObserver<PO.Operation> responseObserver
    ) {
        return supplier -> callHandler.invoke(responseObserver,
                () -> operationConverter.operation(supplier.get()));
    }

}
