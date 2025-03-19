package yandex.cloud.ti.rm.client;

import java.util.List;
import java.util.Map;

import com.google.protobuf.Any;
import com.google.protobuf.Empty;
import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;
import yandex.cloud.auth.bindings.AccessBinding;
import yandex.cloud.auth.bindings.AccessBindingAction;
import yandex.cloud.auth.bindings.AccessSubject;
import yandex.cloud.converter.AnyConverter;
import yandex.cloud.iam.grpc.remoteoperation.RemoteOperationUtils;
import yandex.cloud.iam.grpc.remoteoperation.StatusOrEmptyOp;
import yandex.cloud.iam.grpc.remoteoperation.StatusOrOperation;
import yandex.cloud.priv.access.PA;
import yandex.cloud.priv.iam.v1.PPS;
import yandex.cloud.priv.resourcemanager.v1.CloudServiceGrpc;
import yandex.cloud.priv.resourcemanager.v1.FolderServiceGrpc;
import yandex.cloud.priv.resourcemanager.v1.OperationServiceGrpc;
import yandex.cloud.priv.resourcemanager.v1.PC;
import yandex.cloud.priv.resourcemanager.v1.PCS;
import yandex.cloud.priv.resourcemanager.v1.PF;
import yandex.cloud.priv.resourcemanager.v1.PFS;
import yandex.cloud.priv.resourcemanager.v1.POS;
import yandex.cloud.ti.grpc.IdempotencyKeyInterceptor;

class ResourceManagerClientImpl implements ResourceManagerClient {

    private final @NotNull CloudServiceGrpc.CloudServiceBlockingStub cloudServiceStub;
    private final @NotNull FolderServiceGrpc.FolderServiceBlockingStub folderServiceStub;
    private final @NotNull OperationServiceGrpc.OperationServiceBlockingStub operationStub;

    private final @Nullable String idempotencyKey;


    ResourceManagerClientImpl(
            @NotNull CloudServiceGrpc.CloudServiceBlockingStub cloudServiceStub,
            @NotNull FolderServiceGrpc.FolderServiceBlockingStub folderServiceStub,
            @NotNull OperationServiceGrpc.OperationServiceBlockingStub operationStub
    ) {
        this(
                cloudServiceStub,
                folderServiceStub,
                operationStub,
                null
        );
    }

    private ResourceManagerClientImpl(
            @NotNull CloudServiceGrpc.CloudServiceBlockingStub cloudServiceStub,
            @NotNull FolderServiceGrpc.FolderServiceBlockingStub folderServiceStub,
            @NotNull OperationServiceGrpc.OperationServiceBlockingStub operationStub,
            @Nullable String idempotencyKey
    ) {
        this.cloudServiceStub = cloudServiceStub;
        this.folderServiceStub = folderServiceStub;
        this.operationStub = operationStub;
        this.idempotencyKey = idempotencyKey;
    }


    @Override
    public @NotNull ResourceManagerClient withIdempotencyKey(@NotNull String idempotencyKey) {
        return new ResourceManagerClientImpl(
                cloudServiceStub,
                folderServiceStub,
                operationStub,
                idempotencyKey
        );
    }

    @Override
    public @NotNull StatusOrOperation<Cloud> createCloud(
            @Nullable String organizationId,
            @Nullable String name,
            @Nullable String description
    ) {
        return RemoteOperationUtils.callOperation(
                () -> IdempotencyKeyInterceptor.withIdempotencyKey(cloudServiceStub, idempotencyKey)
                        .create(createCloudRequest(
                                organizationId,
                                name,
                                description
                        )),
                Cloud.class,
                ResourceManagerClientImpl::unpackCloud
        );
    }

    @Override
    public @NotNull StatusOrOperation<Cloud> getCloudOperation(@NotNull String operationId) {
        return RemoteOperationUtils.callOperation(
                () -> IdempotencyKeyInterceptor.withIdempotencyKey(operationStub, idempotencyKey)
                        .get(getOperationRequest(
                                operationId
                        )),
                Cloud.class,
                ResourceManagerClientImpl::unpackCloud
        );
    }

    @Override
    public @NotNull StatusOrEmptyOp updateCloudAccessBindings(@NotNull String cloudId, @NotNull List<AccessBindingAction> actions, boolean privateCall) {
        return RemoteOperationUtils.callEmptyOperation(
                () -> IdempotencyKeyInterceptor.withIdempotencyKey(cloudServiceStub, idempotencyKey)
                        .updateAccessBindings(updateAccessBindingsRequest(
                                cloudId,
                                actions,
                                privateCall
                        ))
        );

    }

    @Override
    public @NotNull StatusOrOperation<Folder> createFolder(@NotNull String cloudId, @NotNull String name, String description, Map<String, String> labels) {
        return RemoteOperationUtils.callOperation(
                () -> IdempotencyKeyInterceptor.withIdempotencyKey(folderServiceStub, idempotencyKey)
                        .create(createFolderRequest(
                                cloudId,
                                name,
                                description,
                                labels
                        )),
                Folder.class,
                ResourceManagerClientImpl::unpackFolder
        );
    }

    @Override
    public @NotNull StatusOrOperation<Folder> getFolderOperation(@NotNull String operationId) {
        return RemoteOperationUtils.callOperation(
                () -> IdempotencyKeyInterceptor.withIdempotencyKey(operationStub, idempotencyKey)
                        .get(getOperationRequest(
                                operationId
                        )),
                Folder.class,
                ResourceManagerClientImpl::unpackFolder
        );
    }

    @Override
    public @NotNull StatusOrEmptyOp setAllCloudPermissionStages(
            @NotNull String cloudId
    ) {
        return RemoteOperationUtils.callEmptyOperation(
                () -> IdempotencyKeyInterceptor.withIdempotencyKey(cloudServiceStub, idempotencyKey)
                        .setAllPermissionStages(setAllPermissionStagesRequest(
                                cloudId
                        ))
        );
    }

    @Override
    public @NotNull StatusOrEmptyOp getEmptyOperation(
            @NotNull String operationId
    ) {
        return RemoteOperationUtils.callEmptyOperation(
                () -> IdempotencyKeyInterceptor.withIdempotencyKey(operationStub, idempotencyKey)
                        .get(getOperationRequest(
                                operationId
                        ))
        );
    }

    private static Cloud cloud(
            @NotNull PC.Cloud cloud
    ) {
        return new Cloud(
                cloud.getId(),
                emptyToNull(cloud.getOrganizationId())
        );
    }

    private static @Nullable Cloud unpackCloud(
            @NotNull Any any
    ) {
        return AnyConverter.<Cloud>unpack(any)
                .bind(Empty.class, it -> null)
                .bind(PC.Cloud.class, ResourceManagerClientImpl::cloud)
                .build();
    }

    private static Folder folder(PF.Folder folder) {
        return new Folder(
                folder.getId(),
                folder.getCloudId()
        );
    }

    private static @Nullable Folder unpackFolder(
            @NotNull Any any
    ) {
        return AnyConverter.<Folder>unpack(any)
                .bind(Empty.class, it -> null)
                .bind(PF.Folder.class, ResourceManagerClientImpl::folder)
                .build();
    }

    private static @Nullable String emptyToNull(@NotNull String value) {
        if (value.isEmpty()) {
            return null;
        }
        return value;
    }

    private static PCS.CreateCloudRequest createCloudRequest(
            @Nullable String organizationId,
            @Nullable String name,
            @Nullable String description
    ) {
        PCS.CreateCloudRequest.Builder builder = PCS.CreateCloudRequest.newBuilder();
        if (organizationId != null) {
            builder.setOrganizationId(organizationId);
        }
        if (name != null) {
            builder.setName(name);
        }
        if (description != null) {
            builder.setDescription(description);
        }
        return builder.build();
    }

    private static PFS.CreateFolderRequest createFolderRequest(
            @NotNull String cloudId,
            @Nullable String name,
            @Nullable String description,
            @Nullable Map<String, String> labels
    ) {
        PFS.CreateFolderRequest.Builder builder = PFS.CreateFolderRequest.newBuilder()
                .setCloudId(cloudId);
        if (name != null) {
            builder.setName(name);
        }
        if (description != null) {
            builder.setDescription(description);
        }
        if (labels != null) {
            builder.putAllLabels(labels);
        }
        return builder.build();
    }

    private static POS.GetOperationRequest getOperationRequest(String operationId) {
        return POS.GetOperationRequest.newBuilder()
                .setOperationId(operationId)
                .build();
    }

    private static PA.UpdateAccessBindingsRequest updateAccessBindingsRequest(
            @NotNull String resourceId,
            @NotNull List<AccessBindingAction> actions,
            boolean privateCall
    ) {
        return PA.UpdateAccessBindingsRequest.newBuilder()
                .setResourceId(resourceId)
                .addAllAccessBindingDeltas(actions.stream()
                        .map(ResourceManagerClientImpl::accessBindingDelta)
                        .toList()
                )
                .setPrivateCall(privateCall)
                .build();
    }

    private static PA.AccessBindingDelta accessBindingDelta(
            @NotNull AccessBindingAction action
    ) {
        return PA.AccessBindingDelta.newBuilder()
                .setAction(PA.AccessBindingAction.valueOf(action.getAction().name()))
                .setAccessBinding(accessBinding(action.getAccessBinding()))
                .build();
    }

    private static PA.AccessBinding accessBinding(
            @NotNull AccessBinding binding
    ) {
        return PA.AccessBinding.newBuilder()
                .setRoleId(binding.getRoleId())
                .setSubject(subject(binding.getSubject()))
                .build();
    }

    private static PA.Subject subject(
            @NotNull AccessSubject subject
    ) {
        return PA.Subject.newBuilder()
                .setId(subject.getId())
                .setType(subject.getType())
                .build();
    }

    private static @NotNull PPS.SetAllPermissionStagesRequest setAllPermissionStagesRequest(
            @NotNull String cloudId
    ) {
        return PPS.SetAllPermissionStagesRequest.newBuilder()
                .setResourceId(cloudId)
                .build();
    }

}
