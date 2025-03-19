package yandex.cloud.ti.rm.client;

import java.util.Map;
import java.util.function.Function;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.iam.grpc.remoteoperation.RemoteOperation;
import yandex.cloud.iam.grpc.remoteoperation.RemoteOperationCall;
import yandex.cloud.iam.grpc.remoteoperation.StatusOrOperation;

public class CreateFolderRemoteOpCall extends RemoteOperationCall.BaseCall<Folder> {

    protected CreateFolderRemoteOpCall(
            Function<String, StatusOrOperation<Folder>> createOperation,
            Function<RemoteOperation.Id, StatusOrOperation<Folder>> getOperation
    ) {
        super(createOperation, getOperation);
    }

    public static RemoteOperationCall.BaseCall<Folder> createCreateFolder(
            @NotNull ResourceManagerClient resourceManagerClient,
            @NotNull String cloudId,
            @NotNull String name,
            String description,
            Map<String, String> labels
    ) {
        return new CreateFolderRemoteOpCall(
                idempotencyKey -> resourceManagerClient
                        .withIdempotencyKey(idempotencyKey)
                        .createFolder(cloudId, name, description, labels),
                operationId -> resourceManagerClient
                        .getFolderOperation(operationId.getValue())
        );
    }

}
