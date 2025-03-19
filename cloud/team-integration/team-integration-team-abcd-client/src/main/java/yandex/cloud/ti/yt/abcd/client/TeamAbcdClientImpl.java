package yandex.cloud.ti.yt.abcd.client;

import java.util.Optional;

import io.grpc.Status;
import org.jetbrains.annotations.NotNull;
import yandex.cloud.iam.grpc.client.StatusOrResult;
import yandex.cloud.ti.yt.abc.client.AbcServiceNotFoundException;

class TeamAbcdClientImpl implements TeamAbcdClient {

    private final @NotNull FolderServiceGrpcClient abcFolderClient;


    TeamAbcdClientImpl(
            @NotNull FolderServiceGrpcClient abcFolderClient
    ) {
        this.abcFolderClient = abcFolderClient;
    }


    @Override
    public @NotNull TeamAbcdFolder getAbcdFolder(@NotNull String abcdFolderId) {
        StatusOrResult<TeamAbcdFolder> result = abcFolderClient.getFolder(abcdFolderId);
        // Pass NOT_FOUND to client
        if (result.isStatus() && Status.NOT_FOUND.getCode() == result.getStatusCode()) {
            throw AbcFolderNotFoundException.forId(abcdFolderId);
        }
        // Will throw any other error as INTERNAL
        return result.getResult();
    }

    @Override
    public @NotNull TeamAbcdFolder getDefaultAbcdFolder(long abcServiceId) {
        String pageToken = null;
        Optional<TeamAbcdFolder> defaultAbcFolder;
        do {
            var response = abcFolderClient.listFoldersByService(abcServiceId, pageToken);
            if (response.isStatus() && Status.NOT_FOUND.getCode() == response.getStatusCode()) {
                throw AbcServiceNotFoundException.forAbcId(abcServiceId);
            }

            defaultAbcFolder = response.getResult().stream()
                    .filter(TeamAbcdFolder::defaultForService)
                    .findAny();
            pageToken = response.getResult().getNextPageToken();
        } while (defaultAbcFolder.isEmpty() && pageToken != null);

        return defaultAbcFolder.orElseThrow(() -> AbcFolderNotFoundException.forAbcServiceId(abcServiceId));
    }

}
