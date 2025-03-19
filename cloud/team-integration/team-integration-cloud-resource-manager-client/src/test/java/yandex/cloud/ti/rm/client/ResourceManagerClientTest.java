package yandex.cloud.ti.rm.client;

import java.util.List;
import java.util.UUID;

import com.google.protobuf.Any;
import io.grpc.Metadata;
import io.grpc.inprocess.InProcessChannelBuilder;
import io.grpc.inprocess.InProcessServerBuilder;
import io.grpc.internal.GrpcUtil;
import io.grpc.testing.GrpcCleanupRule;
import org.assertj.core.api.Assertions;
import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import yandex.cloud.auth.bindings.AccessBinding;
import yandex.cloud.auth.bindings.AccessBindingAction;
import yandex.cloud.auth.bindings.AccessSubject;
import yandex.cloud.auth.bindings.FederatedUser;
import yandex.cloud.auth.bindings.UserAccount;
import yandex.cloud.grpc.GrpcHeaders;
import yandex.cloud.iam.grpc.remoteoperation.RemoteOperation;
import yandex.cloud.priv.access.PA;
import yandex.cloud.priv.operation.PO;
import yandex.cloud.priv.resourcemanager.v1.CloudServiceGrpc;
import yandex.cloud.priv.resourcemanager.v1.FolderServiceGrpc;
import yandex.cloud.priv.resourcemanager.v1.OperationServiceGrpc;
import yandex.cloud.priv.resourcemanager.v1.PC;
import yandex.cloud.priv.resourcemanager.v1.PCS;
import yandex.cloud.priv.resourcemanager.v1.PF;
import yandex.cloud.priv.resourcemanager.v1.PFS;
import yandex.cloud.priv.resourcemanager.v1.POS;
import yandex.cloud.ti.grpc.MockGrpcServer;

public class ResourceManagerClientTest {

    @Rule
    public final @NotNull GrpcCleanupRule grpcCleanup = new GrpcCleanupRule();

    private MockGrpcServer mockGrpcServer;
    private ResourceManagerClient resourceManagerClient;


    @Before
    public void createResourceManagerClient() throws Exception {
        mockGrpcServer = new MockGrpcServer();

        String serverName = InProcessServerBuilder.generateName();
        grpcCleanup.register(InProcessServerBuilder
                .forName(serverName)
                .directExecutor()
                .addService(mockGrpcServer.mockService(CloudServiceGrpc.getServiceDescriptor()))
                .addService(mockGrpcServer.mockService(FolderServiceGrpc.getServiceDescriptor()))
                .addService(mockGrpcServer.mockService(OperationServiceGrpc.getServiceDescriptor()))
                .build()
                .start()
        );

        resourceManagerClient = ResourceManagerClientFactory.createResourceManagerClient(
                grpcCleanup.register(ResourceManagerClientFactory.configureChannel(
                        serverName,
                        InProcessChannelBuilder
                                .forName(serverName)
                                .directExecutor()
                )),
                () -> UUID.randomUUID().toString()
        );
    }


    @Test
    public void createCloudWithoutOrganization() {
        Cloud cloud = TestResourceManagerClouds.nextCloud();
        createCloud(cloud);
    }

    @Test
    public void createCloudWithOrganization() {
        Cloud cloud = TestResourceManagerClouds.nextCloud("org");
        createCloud(cloud);
    }

    private void createCloud(@NotNull Cloud cloud) {
        PC.Cloud protoCloud = TestResourceManagerClouds.templateProtoCloud(cloud);
        String operationId = TestResourceManagerOperations.nextOperationId();

        mockGrpcServer.enqueueResponse(
                CloudServiceGrpc.getCreateMethod(),
                () -> PO.Operation.newBuilder()
                        .setId(operationId)
                        .setDone(true)
                        .setResponse(Any.pack(protoCloud))
                        .build()
        );

        String idempotencyKey = "idempotencyKey " + cloud.id();
        RemoteOperation<Cloud> operation = resourceManagerClient
                .withIdempotencyKey(idempotencyKey)
                .createCloud(
                        cloud.organizationId(),
                        protoCloud.getName(),
                        protoCloud.getDescription()
                )
                .getResult();
        Assertions.assertThat(operation.getId().getValue()).isEqualTo(operationId);
        Assertions.assertThat(operation.isDone()).isTrue();

        Cloud created = operation.getStatusOrResult().getResult();
        Assertions.assertThat(created.id()).isEqualTo(cloud.id());
        Assertions.assertThat(created.organizationId()).isEqualTo(cloud.organizationId());

        var requestHolder = mockGrpcServer.takeRequestHolder(CloudServiceGrpc.getCreateMethod());
        PCS.CreateCloudRequest createCloudRequest = requestHolder.request();
        Assertions.assertThat(createCloudRequest.getId()).isEmpty();
        Assertions.assertThat(createCloudRequest.getName()).isEqualTo(protoCloud.getName());
        Assertions.assertThat(createCloudRequest.getDescription()).isEqualTo(protoCloud.getDescription());
        Assertions.assertThat(createCloudRequest.getOrganizationId()).isEqualTo(protoCloud.getOrganizationId());
        assertRequestHeaders(requestHolder.headers(), idempotencyKey);
    }

    @Test
    public void createCloudWhenOperationIsNotDone() {
        Cloud cloud = TestResourceManagerClouds.nextCloud();
        PC.Cloud protoCloud = TestResourceManagerClouds.templateProtoCloud(cloud);
        String operationId = TestResourceManagerOperations.nextOperationId();

        mockGrpcServer.enqueueResponse(
                CloudServiceGrpc.getCreateMethod(),
                () -> PO.Operation.newBuilder()
                        .setId(operationId)
                        .setDone(false)
                        .build()
        );

        String idempotencyKey = "idempotencyKey " + cloud.id();
        RemoteOperation<Cloud> operation = resourceManagerClient
                .withIdempotencyKey(idempotencyKey)
                .createCloud(
                        cloud.organizationId(),
                        protoCloud.getName(),
                        protoCloud.getDescription()
                )
                .getResult();
        Assertions.assertThat(operation.getId().getValue()).isEqualTo(operationId);
        Assertions.assertThat(operation.isDone()).isFalse();
    }

    @Test
    public void getCloudOperation() {
        Cloud cloud = TestResourceManagerClouds.nextCloud();
        String operationId = TestResourceManagerOperations.nextOperationId();

        mockGrpcServer.enqueueResponse(
                OperationServiceGrpc.getGetMethod(),
                () -> PO.Operation.newBuilder()
                        .setId(operationId)
                        .setDone(true)
                        .setResponse(Any.pack(TestResourceManagerClouds.templateProtoCloud(cloud)))
                        .build()
        );

        RemoteOperation<Cloud> operation = resourceManagerClient.getCloudOperation(operationId)
                .getResult();
        Assertions.assertThat(operation.getId().getValue()).isEqualTo(operationId);
        Assertions.assertThat(operation.isDone()).isTrue();

        Cloud created = operation.getStatusOrResult().getResult();
        Assertions.assertThat(created.id()).isEqualTo(cloud.id());

        var requestHolder = mockGrpcServer.takeRequestHolder(OperationServiceGrpc.getGetMethod());
        POS.GetOperationRequest getOperationRequest = requestHolder.request();
        Assertions.assertThat(getOperationRequest.getOperationId()).isEqualTo(operationId);
        assertRequestHeaders(requestHolder.headers(), null);
    }

    @Test
    public void getCloudOperationWhenOperationIsNotDone() {
        String operationId = TestResourceManagerOperations.nextOperationId();

        mockGrpcServer.enqueueResponse(
                OperationServiceGrpc.getGetMethod(),
                () -> PO.Operation.newBuilder()
                        .setId(operationId)
                        .setDone(false)
                        .build()
        );

        RemoteOperation<Cloud> operation = resourceManagerClient.getCloudOperation(operationId)
                .getResult();
        Assertions.assertThat(operation.getId().getValue()).isEqualTo(operationId);
        Assertions.assertThat(operation.isDone()).isFalse();
    }

    @Test
    public void updateCloudAccessBindings() {
        Cloud cloud = TestResourceManagerClouds.nextCloud();
        String operationId = TestResourceManagerOperations.nextOperationId();
        boolean privateCall = true;

        mockGrpcServer.enqueueResponse(
                CloudServiceGrpc.getUpdateAccessBindingsMethod(),
                () -> PO.Operation.newBuilder()
                        .setId(operationId)
                        .setDone(true)
                        .build()
        );

        RemoteOperation<RemoteOperation.Empty> operation = resourceManagerClient
                .updateCloudAccessBindings(
                        cloud.id(),
                        List.of(
                                AccessBindingAction.builder()
                                        .action(AccessBindingAction.Action.ADD)
                                        .accessBinding(AccessBinding.builder()
                                                .roleId("roleId1")
                                                .subject(AccessSubject.create(
                                                        "subjectId1",
                                                        UserAccount.TYPE
                                                ))
                                                .build()
                                        )
                                        .build(),
                                AccessBindingAction.builder()
                                        .action(AccessBindingAction.Action.REMOVE)
                                        .accessBinding(AccessBinding.builder()
                                                .roleId("roleId2")
                                                .subject(AccessSubject.create(
                                                        "subjectId2",
                                                        FederatedUser.TYPE
                                                ))
                                                .build()
                                        )
                                        .build()
                        ),
                        privateCall
                )
                .getResult();
        Assertions.assertThat(operation.getId().getValue()).isEqualTo(operationId);
        Assertions.assertThat(operation.isDone()).isTrue();

        var requestHolder = mockGrpcServer.takeRequestHolder(CloudServiceGrpc.getUpdateAccessBindingsMethod());
        PA.UpdateAccessBindingsRequest updateAccessBindingsRequest = requestHolder.request();
        Assertions.assertThat(updateAccessBindingsRequest.getResourceId()).isEqualTo(cloud.id());
        PA.AccessBindingDelta accessBindingDelta1 = updateAccessBindingsRequest.getAccessBindingDeltasList().get(0);
        Assertions.assertThat(accessBindingDelta1.getAction()).isEqualTo(PA.AccessBindingAction.ADD);
        Assertions.assertThat(accessBindingDelta1.getAccessBinding().getRoleId()).isEqualTo("roleId1");
        Assertions.assertThat(accessBindingDelta1.getAccessBinding().getSubject().getId()).isEqualTo("subjectId1");
        PA.AccessBindingDelta accessBindingDelta2 = updateAccessBindingsRequest.getAccessBindingDeltasList().get(1);
        Assertions.assertThat(accessBindingDelta2.getAction()).isEqualTo(PA.AccessBindingAction.REMOVE);
        Assertions.assertThat(accessBindingDelta2.getAccessBinding().getRoleId()).isEqualTo("roleId2");
        Assertions.assertThat(accessBindingDelta2.getAccessBinding().getSubject().getId()).isEqualTo("subjectId2");
        Assertions.assertThat(updateAccessBindingsRequest.getPrivateCall()).isEqualTo(privateCall);
        assertRequestHeaders(requestHolder.headers(), null);
    }


    @Test
    public void createFolder() {
        Cloud cloud = TestResourceManagerClouds.nextCloud();
        Folder folder = TestResourceManagerFolders.nextFolder(cloud);
        PF.Folder protoFolder = TestResourceManagerFolders.templateProtoFolder(folder);
        String operationId = TestResourceManagerOperations.nextOperationId();

        mockGrpcServer.enqueueResponse(
                FolderServiceGrpc.getCreateMethod(),
                () -> PO.Operation.newBuilder()
                        .setId(operationId)
                        .setDone(true)
                        .setResponse(Any.pack(protoFolder))
                        .build()
        );

        String idempotencyKey = "idempotencyKey " + cloud.id();
        RemoteOperation<Folder> operation = resourceManagerClient
                .withIdempotencyKey(idempotencyKey)
                .createFolder(
                        cloud.id(),
                        protoFolder.getName(),
                        protoFolder.getDescription(),
                        protoFolder.getLabelsMap()
                )
                .getResult();
        Assertions.assertThat(operation.getId().getValue()).isEqualTo(operationId);
        Assertions.assertThat(operation.isDone()).isTrue();

        Folder created = operation.getStatusOrResult().getResult();
        Assertions.assertThat(created.id()).isEqualTo(folder.id());

        var requestHolder = mockGrpcServer.takeRequestHolder(FolderServiceGrpc.getCreateMethod());
        PFS.CreateFolderRequest createFolderRequest = requestHolder.request();
        Assertions.assertThat(createFolderRequest.getId()).isEmpty();
        Assertions.assertThat(createFolderRequest.getCloudId()).isEqualTo(protoFolder.getCloudId());
        Assertions.assertThat(createFolderRequest.getName()).isEqualTo(protoFolder.getName());
        Assertions.assertThat(createFolderRequest.getDescription()).isEqualTo(protoFolder.getDescription());
        Assertions.assertThat(createFolderRequest.getLabelsMap()).isEqualTo(protoFolder.getLabelsMap());
        assertRequestHeaders(requestHolder.headers(), idempotencyKey);
    }

    @Test
    public void createFolderWhenOperationIsNotDone() {
        Cloud cloud = TestResourceManagerClouds.nextCloud();
        Folder folder = TestResourceManagerFolders.nextFolder(cloud);
        PF.Folder protoFolder = TestResourceManagerFolders.templateProtoFolder(folder);
        String operationId = TestResourceManagerOperations.nextOperationId();

        mockGrpcServer.enqueueResponse(
                FolderServiceGrpc.getCreateMethod(),
                () -> PO.Operation.newBuilder()
                        .setId(operationId)
                        .setDone(false)
                        .build()
        );

        String idempotencyKey = "idempotencyKey " + cloud.id();
        RemoteOperation<Folder> operation = resourceManagerClient
                .withIdempotencyKey(idempotencyKey)
                .createFolder(
                        cloud.id(),
                        protoFolder.getName(),
                        protoFolder.getDescription(),
                        protoFolder.getLabelsMap()
                )
                .getResult();
        Assertions.assertThat(operation.getId().getValue()).isEqualTo(operationId);
        Assertions.assertThat(operation.isDone()).isFalse();
    }

    @Test
    public void getFolderOperation() {
        Cloud cloud = TestResourceManagerClouds.nextCloud();
        Folder folder = TestResourceManagerFolders.nextFolder(cloud);
        String operationId = TestResourceManagerOperations.nextOperationId();

        mockGrpcServer.enqueueResponse(
                OperationServiceGrpc.getGetMethod(),
                () -> PO.Operation.newBuilder()
                        .setId(operationId)
                        .setDone(true)
                        .setResponse(Any.pack(TestResourceManagerFolders.templateProtoFolder(folder)))
                        .build()
        );

        RemoteOperation<Folder> operation = resourceManagerClient.getFolderOperation(operationId)
                .getResult();
        Assertions.assertThat(operation.getId().getValue()).isEqualTo(operationId);
        Assertions.assertThat(operation.isDone()).isTrue();

        Folder created = operation.getStatusOrResult().getResult();
        Assertions.assertThat(created.id()).isEqualTo(folder.id());

        var requestHolder = mockGrpcServer.takeRequestHolder(OperationServiceGrpc.getGetMethod());
        POS.GetOperationRequest getOperationRequest = requestHolder.request();
        Assertions.assertThat(getOperationRequest.getOperationId()).isEqualTo(operationId);
        assertRequestHeaders(requestHolder.headers(), null);
    }

    @Test
    public void getFolderOperationWhenOperationIsNotDone() {
        String operationId = TestResourceManagerOperations.nextOperationId();

        mockGrpcServer.enqueueResponse(
                OperationServiceGrpc.getGetMethod(),
                () -> PO.Operation.newBuilder()
                        .setId(operationId)
                        .setDone(false)
                        .build()
        );

        RemoteOperation<Folder> operation = resourceManagerClient.getFolderOperation(operationId)
                .getResult();
        Assertions.assertThat(operation.getId().getValue()).isEqualTo(operationId);
        Assertions.assertThat(operation.isDone()).isFalse();
    }

    private static void assertRequestHeaders(
            @NotNull Metadata headers,
            @Nullable String idempotencyKey
    ) {
        Assertions.assertThat(headers.get(GrpcHeaders.AUTHORIZATION)).startsWith("Bearer ");
        Assertions.assertThat(headers.get(GrpcHeaders.X_REQUEST_ID)).isNotEmpty();
        Assertions.assertThat(headers.get(GrpcHeaders.IDEMPOTENCY_KEY)).isEqualTo(idempotencyKey);
        Assertions.assertThat(headers.get(GrpcUtil.TIMEOUT_KEY)).isNotZero();
    }

}
