package yandex.cloud.ti.yt.abcd.client;

import java.util.ArrayList;
import java.util.List;

import org.jetbrains.annotations.NotNull;

public class MockTeamAbcdClient implements TeamAbcdClient {

    private final @NotNull List<TeamAbcdFolder> folders = new ArrayList<>();


    @Override
    public @NotNull TeamAbcdFolder getAbcdFolder(@NotNull String abcdFolderId) {
        return folders.stream()
                .filter(it -> it.id().equals(abcdFolderId))
                .findFirst()
                .orElseThrow(() -> AbcFolderNotFoundException.forId(abcdFolderId));
    }

    @Override
    public @NotNull TeamAbcdFolder getDefaultAbcdFolder(long abcServiceId) {
        return folders.stream()
                .filter(it -> it.abcServiceId() == abcServiceId)
                .filter(TeamAbcdFolder::defaultForService)
                .findFirst()
                .orElseThrow(() -> AbcFolderNotFoundException.forAbcServiceId(abcServiceId));
    }


    public void addFolder(@NotNull TeamAbcdFolder folder) {
        folders.add(folder);
    }

    public void clearFolders() {
        folders.clear();
    }

}
