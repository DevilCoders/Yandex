package yandex.cloud.ti.abc;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.ti.yt.abc.client.TestTeamAbcServices;
import yandex.cloud.ti.yt.abcd.client.TeamAbcdFolder;
import yandex.cloud.ti.yt.abcd.client.TestTeamAbcdFolders;

public final class TestAbcServiceClouds {

    public static @NotNull String templateAbcServiceCloudId(
            @NotNull TeamAbcdFolder abcdFolder
    ) {
        return abcdFolder.id() + "-cloud";
    }

    public static @NotNull String templateAbcServiceDefaultFolderId(
            @NotNull TeamAbcdFolder abcdFolder
    ) {
        return abcdFolder.id() + "-default-folder";
    }


    public static @NotNull AbcServiceCloud nextAbcServiceCloud() {
        return templateAbcServiceCloud(TestTeamAbcdFolders.nextAbcdFolder(TestTeamAbcServices.nextAbcServiceId()));
    }

    public static @NotNull AbcServiceCloud templateAbcServiceCloud(
            @NotNull TeamAbcdFolder abcdFolder
    ) {
        return new AbcServiceCloud(
                abcdFolder.abcServiceId(),
                TestTeamAbcServices.templateAbcServiceSlug(abcdFolder.abcServiceId()),
                abcdFolder.id(),
                templateAbcServiceCloudId(abcdFolder),
                templateAbcServiceDefaultFolderId(abcdFolder)
        );
    }


    private TestAbcServiceClouds() {
    }

}
