package yandex.cloud.team.integration.abc;

import io.grpc.StatusRuntimeException;
import lombok.extern.log4j.Log4j2;
import org.jetbrains.annotations.NotNull;
import yandex.cloud.iam.grpc.remoteoperation.RemoteOperationCall;
import yandex.cloud.iam.grpc.remoteoperation.RemoteOperationProcess;
import yandex.cloud.iam.grpc.remoteoperation.RemoteOperationProcessState;
import yandex.cloud.iam.grpc.remoteoperation.RemoteOperationRetryConfig;
import yandex.cloud.ti.rm.client.Cloud;

@Log4j2
public class CreateCloudRemoteOperationProcess extends RemoteOperationProcess<Cloud> {

    public CreateCloudRemoteOperationProcess(
            @NotNull RemoteOperationProcessState state,
            @NotNull RemoteOperationCall.Call<Cloud> call,
            @NotNull RemoteOperationRetryConfig retryConfig
    ) {
        super(state, call, retryConfig);
    }

    @Override
    protected AttemptStatus classifyAttemptStatus(
            StatusRuntimeException responseStatus
    ) {
        return switch (responseStatus.getStatus().getCode()) {
            case ALREADY_EXISTS -> AttemptStatus.SUCCEEDED;
            default -> super.classifyAttemptStatus(responseStatus);
        };
    }

}
