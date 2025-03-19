package yandex.cloud.ti.rm.client;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;
import yandex.cloud.auth.bindings.AccessBindingAction;
import yandex.cloud.iam.grpc.remoteoperation.RemoteOperation;
import yandex.cloud.iam.grpc.remoteoperation.StatusOrEmptyOp;
import yandex.cloud.iam.grpc.remoteoperation.StatusOrOperation;

public class MockResourceManagerClient implements ResourceManagerClient {

    private final @NotNull List<AccessBindingsRequest> accessBindingsRequests = new ArrayList<>();


    @Override
    public @NotNull ResourceManagerClient withIdempotencyKey(@NotNull String idempotencyKey) {
        return this;
    }

    @Override
    public @NotNull StatusOrOperation<Cloud> createCloud(
            @Nullable String organizationId,
            @Nullable String name,
            @Nullable String description
    ) {
        return StatusOrOperation.result(RemoteOperation.success(
                RemoteOperation.Id.of("id"),
                new Cloud(
                        TestResourceManagerClouds.nextCloudId(),
                        organizationId
                )
        ));
    }

    @Override
    public @NotNull StatusOrOperation<Cloud> getCloudOperation(@NotNull String operationId) {
        throw new AssertionError();
    }

    @Override
    public @NotNull StatusOrEmptyOp updateCloudAccessBindings(@NotNull String cloudId, @NotNull List<AccessBindingAction> actions, boolean privateCall) {
        accessBindingsRequests.add(new AccessBindingsRequest(
                "resource-manager.cloud",
                cloudId,
                actions,
                privateCall
        ));
        return StatusOrEmptyOp.result(RemoteOperation.success(RemoteOperation.Id.of("id")));
    }

    @Override
    public @NotNull StatusOrOperation<Folder> createFolder(@NotNull String cloudId, @NotNull String name, String description, Map<String, String> labels) {
        return StatusOrOperation.result(RemoteOperation.success(
                RemoteOperation.Id.of("id"),
                new Folder(
                        TestResourceManagerFolders.nextFolderId(cloudId),
                        cloudId
                )
        ));
    }

    @Override
    public @NotNull StatusOrOperation<Folder> getFolderOperation(@NotNull String operationId) {
        throw new AssertionError();
    }

    @Override
    public @NotNull StatusOrEmptyOp setAllCloudPermissionStages(@NotNull String cloudId) {
        return StatusOrEmptyOp.result(RemoteOperation.success(RemoteOperation.Id.of("id")));
    }

    @Override
    public @NotNull StatusOrEmptyOp getEmptyOperation(@NotNull String operationId) {
        throw new AssertionError();
    }

    public @NotNull List<AccessBindingsRequest> getAccessBindingsRequests() {
        return accessBindingsRequests;
    }

    public void clearAccessBindingsRequests() {
        accessBindingsRequests.clear();
    }

    public record AccessBindingsRequest(
            @NotNull String resourceType,
            @NotNull String resourceId,
            @NotNull List<AccessBindingAction> actions,
            boolean privateCall
    ) {}

}
