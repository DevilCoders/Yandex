package yandex.cloud.team.integration.abc;

import io.grpc.StatusRuntimeException;
import lombok.extern.log4j.Log4j2;
import org.jetbrains.annotations.NotNull;
import yandex.cloud.iam.grpc.remoteoperation.RemoteOperationCall;
import yandex.cloud.iam.grpc.remoteoperation.RemoteOperationProcess;
import yandex.cloud.iam.grpc.remoteoperation.RemoteOperationProcessState;
import yandex.cloud.iam.grpc.remoteoperation.RemoteOperationRetryConfig;
import yandex.cloud.ti.rm.client.Folder;

@Log4j2
public class CreateFolderRemoteOperationProcess extends RemoteOperationProcess<Folder> {

    public CreateFolderRemoteOperationProcess(
            @NotNull RemoteOperationProcessState state,
            @NotNull RemoteOperationCall.Call<Folder> call,
            @NotNull RemoteOperationRetryConfig retryConfig
    ) {
        super(state, call, retryConfig);
    }

    @Override
    protected AttemptStatus classifyAttemptStatus(StatusRuntimeException responseStatus) {
        return switch (responseStatus.getStatus().getCode()) {
            case ALREADY_EXISTS -> AttemptStatus.SUCCEEDED;
            // We should retry this, due to lag in cloud status refresh in access service
            case PERMISSION_DENIED -> AttemptStatus.RETRY_REQUIRED;
            default -> super.classifyAttemptStatus(responseStatus);
        };
    }

}
