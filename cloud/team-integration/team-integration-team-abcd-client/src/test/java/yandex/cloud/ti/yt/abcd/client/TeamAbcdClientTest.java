package yandex.cloud.ti.yt.abcd.client;

import io.grpc.ManagedChannel;
import io.grpc.Status;
import io.grpc.StatusRuntimeException;
import io.grpc.inprocess.InProcessChannelBuilder;
import io.grpc.inprocess.InProcessServerBuilder;
import io.grpc.testing.GrpcCleanupRule;
import org.assertj.core.api.Assertions;
import org.jetbrains.annotations.NotNull;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import yandex.cloud.ti.grpc.MockGrpcServer;
import yandex.cloud.ti.yt.abc.client.AbcServiceNotFoundException;
import yandex.cloud.ti.yt.abc.client.TeamAbcService;
import yandex.cloud.ti.yt.abc.client.TestTeamAbcServices;

import ru.yandex.intranet.d.backend.service.proto.Folder;
import ru.yandex.intranet.d.backend.service.proto.FolderServiceGrpc;
import ru.yandex.intranet.d.backend.service.proto.FolderType;
import ru.yandex.intranet.d.backend.service.proto.FoldersPageToken;
import ru.yandex.intranet.d.backend.service.proto.ListFoldersByServiceResponse;

public class TeamAbcdClientTest {

    @Rule
    public final @NotNull GrpcCleanupRule grpcCleanup = new GrpcCleanupRule();

    private MockGrpcServer mockGrpcServer;
    private TeamAbcdClient teamAbcdClient;


    @Before
    public void createTeamAbcdClient() throws Exception {
        mockGrpcServer = new MockGrpcServer();

        String serverName = InProcessServerBuilder.generateName();
        grpcCleanup.register(InProcessServerBuilder
                .forName(serverName)
                .directExecutor()
                .addService(mockGrpcServer.mockService(FolderServiceGrpc.getServiceDescriptor()))
                .build()
                .start()
        );
        ManagedChannel channel = grpcCleanup.register(InProcessChannelBuilder
                .forName(serverName)
                .directExecutor()
                .build()
        );

        teamAbcdClient = new TeamAbcdClientImpl(new FolderServiceGrpcClient(
                FolderServiceGrpc.newBlockingStub(channel)
        ));
    }


    @Test
    public void getAbcdFolder() {
        TeamAbcdFolder teamAbcdFolder = TestTeamAbcdFolders.nextAbcdFolder(TestTeamAbcServices.nextAbcService());
        mockGrpcServer.enqueueResponse(
                FolderServiceGrpc.getGetFolderMethod(),
                () -> Folder.newBuilder()
                        .setFolderId(teamAbcdFolder.id())
                        .setServiceId(teamAbcdFolder.abcServiceId())
                        .setDisplayName(teamAbcdFolder.name())
                        .setFolderType(FolderType.COMMON_DEFAULT_FOR_SERVICE)
                        .build()
        );
        Assertions.assertThat(teamAbcdClient.getAbcdFolder(teamAbcdFolder.id()))
                .isEqualTo(teamAbcdFolder);
    }

    @Test
    public void getAbcdFolderWhenGetFolderThrowsNotFound() {
        TeamAbcdFolder teamAbcdFolder = TestTeamAbcdFolders.nextAbcdFolder(TestTeamAbcServices.nextAbcService());
        mockGrpcServer.enqueueResponse(
                FolderServiceGrpc.getGetFolderMethod(),
                () -> {
                    throw Status.NOT_FOUND.asRuntimeException();
                }
        );
        Assertions.assertThatExceptionOfType(AbcFolderNotFoundException.class)
                .isThrownBy(() ->
                        teamAbcdClient.getAbcdFolder(teamAbcdFolder.id())
                )
                .withMessage("ABC folder with id '%s' does not exist",
                        teamAbcdFolder.id()
                );
    }

    @Test
    public void getAbcdFolderWhenGetFolderThrowsInternal() {
        TeamAbcdFolder teamAbcdFolder = TestTeamAbcdFolders.nextAbcdFolder(TestTeamAbcServices.nextAbcService());
        mockGrpcServer.enqueueResponse(
                FolderServiceGrpc.getGetFolderMethod(),
                () -> {
                    throw Status.INTERNAL.asRuntimeException();
                }
        );
        Assertions.assertThatExceptionOfType(StatusRuntimeException.class)
                .isThrownBy(() ->
                        teamAbcdClient.getAbcdFolder(teamAbcdFolder.id())
                );
    }

    @Test
    public void getDefaultAbcdFolder() {
        TeamAbcdFolder teamAbcdFolder = TestTeamAbcdFolders.nextAbcdFolder(TestTeamAbcServices.nextAbcService());
        mockGrpcServer.enqueueResponse(
                FolderServiceGrpc.getListFoldersByServiceMethod(),
                () -> ListFoldersByServiceResponse.newBuilder()
                        .addFolders(Folder.newBuilder()
                                .setFolderId(teamAbcdFolder.id())
                                .setServiceId(teamAbcdFolder.abcServiceId())
                                .setDisplayName(teamAbcdFolder.name())
                                .setFolderType(FolderType.COMMON_DEFAULT_FOR_SERVICE)
                                .build()
                        )
                        .build()
        );
        Assertions.assertThat(teamAbcdClient.getDefaultAbcdFolder(teamAbcdFolder.abcServiceId()))
                .isEqualTo(teamAbcdFolder);
    }

    @Test
    public void getDefaultAbcdFolderWithPagination() {
        TeamAbcService abcService = TestTeamAbcServices.nextAbcService();
        TeamAbcdFolder teamAbcdFolder1 = TestTeamAbcdFolders.nextAbcdFolder(abcService);
        TeamAbcdFolder teamAbcdFolder2 = TestTeamAbcdFolders.nextAbcdFolder(abcService);
        mockGrpcServer.enqueueResponse(
                FolderServiceGrpc.getListFoldersByServiceMethod(),
                () -> ListFoldersByServiceResponse.newBuilder()
                        .addFolders(Folder.newBuilder()
                                .setFolderId(teamAbcdFolder1.id())
                                .setServiceId(teamAbcdFolder1.abcServiceId())
                                .setDisplayName(teamAbcdFolder1.name())
                                .setFolderType(FolderType.COMMON)
                                .build()
                        )
                        .setNextPageToken(FoldersPageToken.newBuilder()
                                .setToken("next")
                                .build()
                        )
                        .build()
        );
        mockGrpcServer.enqueueResponse(
                FolderServiceGrpc.getListFoldersByServiceMethod(),
                () -> ListFoldersByServiceResponse.newBuilder()
                        .addFolders(Folder.newBuilder()
                                .setFolderId(teamAbcdFolder2.id())
                                .setServiceId(teamAbcdFolder2.abcServiceId())
                                .setDisplayName(teamAbcdFolder2.name())
                                .setFolderType(FolderType.COMMON_DEFAULT_FOR_SERVICE)
                                .build()
                        )
                        .build()
        );
        Assertions.assertThat(teamAbcdClient.getDefaultAbcdFolder(abcService.id()))
                .isEqualTo(teamAbcdFolder2);
    }

    @Test
    public void getDefaultAbcdFolderWhenListFoldersByServiceReturnsEmptyList() {
        TeamAbcdFolder teamAbcdFolder = TestTeamAbcdFolders.nextAbcdFolder(TestTeamAbcServices.nextAbcService());
        mockGrpcServer.enqueueResponse(
                FolderServiceGrpc.getListFoldersByServiceMethod(),
                () -> ListFoldersByServiceResponse.newBuilder()
                        .build()
        );
        Assertions.assertThatExceptionOfType(AbcFolderNotFoundException.class)
                .isThrownBy(() ->
                        teamAbcdClient.getDefaultAbcdFolder(teamAbcdFolder.abcServiceId())
                )
                .withMessage("ABC service with id %s has no default folder",
                        teamAbcdFolder.abcServiceId()
                );
    }

    @Test
    public void getDefaultAbcdFolderWhenListFoldersByServiceThrowsNotFound() {
        TeamAbcdFolder teamAbcdFolder = TestTeamAbcdFolders.nextAbcdFolder(TestTeamAbcServices.nextAbcService());
        mockGrpcServer.enqueueResponse(
                FolderServiceGrpc.getListFoldersByServiceMethod(),
                () -> {
                    throw Status.NOT_FOUND.asRuntimeException();
                }
        );
        Assertions.assertThatExceptionOfType(AbcServiceNotFoundException.class)
                .isThrownBy(() ->
                        teamAbcdClient.getDefaultAbcdFolder(teamAbcdFolder.abcServiceId())
                )
                .withMessage("ABC service for id %s not found",
                        teamAbcdFolder.abcServiceId()
                );
    }

    @Test
    public void getDefaultAbcdFolderWhenListFoldersByServiceThrowsInternal() {
        TeamAbcdFolder teamAbcdFolder = TestTeamAbcdFolders.nextAbcdFolder(TestTeamAbcServices.nextAbcService());
        mockGrpcServer.enqueueResponse(
                FolderServiceGrpc.getListFoldersByServiceMethod(),
                () -> {
                    throw Status.INTERNAL.asRuntimeException();
                }
        );
        Assertions.assertThatExceptionOfType(StatusRuntimeException.class)
                .isThrownBy(() ->
                        teamAbcdClient.getDefaultAbcdFolder(teamAbcdFolder.abcServiceId())
                );
    }

}
