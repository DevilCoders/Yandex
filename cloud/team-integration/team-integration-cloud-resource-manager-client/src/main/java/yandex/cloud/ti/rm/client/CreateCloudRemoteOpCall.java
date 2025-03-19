package yandex.cloud.ti.rm.client;

import java.util.function.Function;

import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;
import yandex.cloud.iam.grpc.remoteoperation.RemoteOperation;
import yandex.cloud.iam.grpc.remoteoperation.RemoteOperationCall;
import yandex.cloud.iam.grpc.remoteoperation.StatusOrOperation;

public class CreateCloudRemoteOpCall extends RemoteOperationCall.BaseCall<Cloud> {

    protected CreateCloudRemoteOpCall(
            Function<String, StatusOrOperation<Cloud>> createOperation,
            Function<RemoteOperation.Id, StatusOrOperation<Cloud>> getOperation
    ) {
        super(createOperation, getOperation);
    }

    public static RemoteOperationCall.BaseCall<Cloud> createCreateCloud(
            @NotNull ResourceManagerClient resourceManagerClient,
            @Nullable String organizationId,
            @Nullable String name,
            @Nullable String description
    ) {
        return new CreateCloudRemoteOpCall(
                idempotencyKey -> resourceManagerClient
                        .withIdempotencyKey(idempotencyKey)
                        .createCloud(organizationId, name, description),
                operationId -> resourceManagerClient
                        .getCloudOperation(operationId.getValue())
        );
    }

}
