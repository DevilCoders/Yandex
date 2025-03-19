package yandex.cloud.team.integration.abc;

import io.grpc.StatusRuntimeException;
import org.jetbrains.annotations.NotNull;
import yandex.cloud.iam.grpc.remoteoperation.RemoteOperation;
import yandex.cloud.iam.grpc.remoteoperation.RemoteOperationCall;
import yandex.cloud.iam.grpc.remoteoperation.RemoteOperationProcess;
import yandex.cloud.iam.grpc.remoteoperation.RemoteOperationProcessState;
import yandex.cloud.iam.grpc.remoteoperation.RemoteOperationRetryConfig;

public class SetAllCloudPermissionStagesRemoteOperationProcess extends RemoteOperationProcess<RemoteOperation.Empty> {

    public SetAllCloudPermissionStagesRemoteOperationProcess(
            @NotNull RemoteOperationProcessState state,
            @NotNull RemoteOperationCall.Call<RemoteOperation.Empty> call,
            @NotNull RemoteOperationRetryConfig retryConfig
    ) {
        super(state, call, retryConfig);
    }

    @Override
    protected AttemptStatus classifyAttemptStatus(
            StatusRuntimeException responseStatus
    ) {
        return switch (responseStatus.getStatus().getCode()) {
            // We should retry this, due to lag in cloud status refresh in access service
            case PERMISSION_DENIED -> AttemptStatus.RETRY_REQUIRED;
            default -> super.classifyAttemptStatus(responseStatus);
        };
    }

}
