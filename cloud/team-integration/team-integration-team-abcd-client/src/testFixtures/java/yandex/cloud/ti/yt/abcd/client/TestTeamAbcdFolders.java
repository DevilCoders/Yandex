package yandex.cloud.ti.yt.abcd.client;

import java.util.concurrent.atomic.AtomicLong;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.ti.yt.abc.client.TeamAbcService;
import yandex.cloud.ti.yt.abc.client.TestTeamAbcServices;

public final class TestTeamAbcdFolders {

    private static final @NotNull AtomicLong abcdFolderIdSequence = new AtomicLong();


    public static @NotNull String nextAbcdFolderId(long abcServiceId) {
        return templateAbcdFolderId(abcServiceId, abcdFolderIdSequence.incrementAndGet());
    }

    private static @NotNull String templateAbcdFolderId(long abcServiceId, long abcdFolderIdSeq) {
        return "%s-folder-%d".formatted(
                TestTeamAbcServices.templateAbcServiceSlug(abcServiceId),
                abcdFolderIdSeq
        );
    }


    public static @NotNull TeamAbcdFolder nextAbcdFolder(
            @NotNull TeamAbcService abcService
    ) {
        return nextAbcdFolder(
                abcService.id()
        );
    }

    public static @NotNull TeamAbcdFolder nextAbcdFolder(
            long abcServiceId
    ) {
        return templateAbcdFolder(
                abcServiceId,
                nextAbcdFolderId(abcServiceId)
        );
    }

    public static @NotNull TeamAbcdFolder nextAbcdFolder(
            @NotNull TeamAbcService abcService,
            boolean defaultForService
    ) {
        return nextAbcdFolder(
                abcService.id(),
                defaultForService
        );
    }

    public static @NotNull TeamAbcdFolder nextAbcdFolder(
            long abcServiceId,
            boolean defaultForService
    ) {
        return templateAbcdFolder(
                abcServiceId,
                nextAbcdFolderId(abcServiceId),
                defaultForService
        );
    }

    public static @NotNull TeamAbcdFolder templateAbcdFolder(
            long abcServiceId,
            @NotNull String abcdFolderId
    ) {
        return templateAbcdFolder(
                abcServiceId,
                abcdFolderId,
                true
        );
    }

    public static @NotNull TeamAbcdFolder templateAbcdFolder(
            long abcServiceId,
            @NotNull String abcdFolderId,
            boolean defaultForService
    ) {
        return new TeamAbcdFolder(
                abcdFolderId,
                abcServiceId,
                abcdFolderId + " name",
                defaultForService
        );
    }


    private TestTeamAbcdFolders() {
    }

}
