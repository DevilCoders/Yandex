package yandex.cloud.team.integration.abc;

import lombok.AllArgsConstructor;
import lombok.Getter;
import lombok.With;
import org.jetbrains.annotations.NotNull;
import yandex.cloud.iam.grpc.remoteoperation.RemoteOperationProcessState;

@AllArgsConstructor
@Getter
@With
public class CreateCloudProcessState {

    @NotNull Status status;

    RemoteOperationProcessState createCloudState;

    String cloudId;

    RemoteOperationProcessState setAllCloudPermissionStagesState;

    RemoteOperationProcessState createBillingAccountState;

    RemoteOperationProcessState createFolderState;

    String defaultFolderId;

    public CreateCloudProcessState() {
        this(
                Status.CREATE_CLOUD,
                new RemoteOperationProcessState(),
                null,
                null,
                null,
                null,
                null
        );
    }

    public boolean isDone() {
        return status == Status.SUCCEEDED || status == Status.FAILED;
    }

    public boolean isSucceeded() {
        return status == Status.SUCCEEDED;
    }

    public boolean isFailed() {
        return status == Status.FAILED;
    }

    public enum Status {

        CREATE_CLOUD,
        CREATE_BILLING_ACCOUNT,
        ENABLE_ALPHA_PERMISSIONS,
        CREATE_FOLDER,
        SAVE_RESULT,
        SUCCEEDED,
        FAILED

    }

}
