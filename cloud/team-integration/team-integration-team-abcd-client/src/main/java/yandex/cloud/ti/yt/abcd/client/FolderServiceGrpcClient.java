package yandex.cloud.ti.yt.abcd.client;

import java.util.List;

import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;
import yandex.cloud.iam.grpc.client.StatusOrResult;
import yandex.cloud.iam.grpc.remoteoperation.RemoteOperationUtils;
import yandex.cloud.iam.service.model.ListResultPage;

import ru.yandex.intranet.d.backend.service.proto.Folder;
import ru.yandex.intranet.d.backend.service.proto.FolderServiceGrpc;
import ru.yandex.intranet.d.backend.service.proto.FolderType;
import ru.yandex.intranet.d.backend.service.proto.FoldersLimit;
import ru.yandex.intranet.d.backend.service.proto.FoldersPageToken;
import ru.yandex.intranet.d.backend.service.proto.GetFolderRequest;
import ru.yandex.intranet.d.backend.service.proto.ListFoldersByServiceRequest;
import ru.yandex.intranet.d.backend.service.proto.ListFoldersByServiceResponse;

class FolderServiceGrpcClient {

    private static final int MAX_ABC_FOLDER_LIST_PAGE_SIZE = 100;

    private final @NotNull FolderServiceGrpc.FolderServiceBlockingStub folderServiceStub;


    FolderServiceGrpcClient(
            @NotNull FolderServiceGrpc.FolderServiceBlockingStub folderServiceStub
    ) {
        this.folderServiceStub = folderServiceStub;
    }


    @NotNull StatusOrResult<TeamAbcdFolder> getFolder(
            @NotNull String abcdFolderId
    ) {
        return RemoteOperationUtils.call(() -> {
            GetFolderRequest request = GetFolderRequest.newBuilder()
                    .setFolderId(abcdFolderId)
                    .build();
            Folder folder = folderServiceStub.getFolder(request);
            return toAbcdFolder(folder);
        });
    }

    @NotNull StatusOrResult<ListResultPage<TeamAbcdFolder>> listFoldersByService(
            long abcServiceId,
            @Nullable String pageToken
    ) {
        return RemoteOperationUtils.call(() -> {
            ListFoldersByServiceRequest.Builder requestBuilder = ListFoldersByServiceRequest.newBuilder()
                    .setServiceId(abcServiceId)
                    .setLimit(FoldersLimit.newBuilder()
                            .setLimit(MAX_ABC_FOLDER_LIST_PAGE_SIZE)
                            .build()
                    );
            if (pageToken != null && !pageToken.isEmpty()) {
                requestBuilder.setPageToken(FoldersPageToken.newBuilder()
                        .setToken(pageToken)
                        .build()
                );
            }
            ListFoldersByServiceRequest request = requestBuilder.build();
            ListFoldersByServiceResponse response = folderServiceStub.listFoldersByService(request);
            List<TeamAbcdFolder> abcdFolders = response.getFoldersList()
                    .stream()
                    .map(this::toAbcdFolder)
                    .toList();
            String token = response.getNextPageToken().getToken();
            if (!token.isEmpty()) {
                return ListResultPage.page(abcdFolders, token);
            } else {
                return ListResultPage.lastPage(abcdFolders);
            }
        });
    }


    private @NotNull TeamAbcdFolder toAbcdFolder(Folder folder) {
        // todo check folder.getDeleted()?
        return new TeamAbcdFolder(
                folder.getFolderId(),
                folder.getServiceId(),
                folder.getDisplayName(),
                folder.getFolderType() == FolderType.COMMON_DEFAULT_FOR_SERVICE
        );
    }

}
