package yandex.cloud.ti.rm.client;

import java.util.List;
import java.util.Map;

import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;
import yandex.cloud.auth.bindings.AccessBindingAction;
import yandex.cloud.iam.grpc.remoteoperation.StatusOrEmptyOp;
import yandex.cloud.iam.grpc.remoteoperation.StatusOrOperation;

public interface ResourceManagerClient {

    @NotNull ResourceManagerClient withIdempotencyKey(
            @NotNull String idempotencyKey
    );

    @NotNull StatusOrOperation<Cloud> createCloud(
            @Nullable String organizationId,
            @Nullable String name,
            @Nullable String description
    );

    @NotNull StatusOrOperation<Cloud> getCloudOperation(
            @NotNull String operationId
    );

    @NotNull StatusOrEmptyOp updateCloudAccessBindings(
            @NotNull String cloudId,
            @NotNull List<AccessBindingAction> actions,
            boolean privateCall
    );

    @NotNull StatusOrOperation<Folder> createFolder(
            @NotNull String cloudId,
            @NotNull String name,
            String description,
            Map<String, String> labels
    );

    @NotNull StatusOrOperation<Folder> getFolderOperation(
            @NotNull String operationId
    );

    @NotNull StatusOrEmptyOp setAllCloudPermissionStages(
            @NotNull String cloudId
    );

    @NotNull StatusOrEmptyOp getEmptyOperation(
            @NotNull String operationId
    );

}
