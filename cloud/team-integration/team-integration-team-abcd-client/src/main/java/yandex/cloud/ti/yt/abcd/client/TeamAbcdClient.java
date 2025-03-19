package yandex.cloud.ti.yt.abcd.client;

import org.jetbrains.annotations.NotNull;

public interface TeamAbcdClient {

    @NotNull TeamAbcdFolder getAbcdFolder(
            @NotNull String abcdFolderId
    );

    @NotNull TeamAbcdFolder getDefaultAbcdFolder(
            long abcServiceId
    );

}
