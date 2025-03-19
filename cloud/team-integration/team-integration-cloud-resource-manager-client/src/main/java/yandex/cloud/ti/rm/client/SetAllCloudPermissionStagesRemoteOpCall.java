package yandex.cloud.ti.rm.client;

import java.util.function.Function;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.iam.grpc.remoteoperation.RemoteOperation;
import yandex.cloud.iam.grpc.remoteoperation.RemoteOperationCall;
import yandex.cloud.iam.grpc.remoteoperation.StatusOrEmptyOp;

public class SetAllCloudPermissionStagesRemoteOpCall extends RemoteOperationCall.BaseCall<RemoteOperation.Empty> {

    protected SetAllCloudPermissionStagesRemoteOpCall(
            Function<String, StatusOrEmptyOp> createOperation,
            Function<RemoteOperation.Id, StatusOrEmptyOp> getOperation
    ) {
        super(createOperation, getOperation);
    }

    public static RemoteOperationCall.BaseCall<RemoteOperation.Empty> createSetAllCloudPermissionStages(
            @NotNull ResourceManagerClient resourceManagerClient,
            @NotNull String cloudId
    ) {
        return new SetAllCloudPermissionStagesRemoteOpCall(
                idempotencyKey -> resourceManagerClient
                        .withIdempotencyKey(idempotencyKey)
                        .setAllCloudPermissionStages(cloudId),
                operationId -> resourceManagerClient
                        .getEmptyOperation(operationId.getValue())
        );
    }

}
