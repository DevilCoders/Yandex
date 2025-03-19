package yandex.cloud.ti.rm.client;

import java.time.Instant;
import java.util.concurrent.atomic.AtomicLong;

import com.google.protobuf.Timestamp;
import org.jetbrains.annotations.NotNull;
import yandex.cloud.priv.resourcemanager.v1.PF;

public final class TestResourceManagerFolders {

    private static final @NotNull AtomicLong folderIdSequence = new AtomicLong();


    public static @NotNull String nextFolderId(
            @NotNull String cloudId
    ) {
        return templateFolderId(
                cloudId,
                folderIdSequence.incrementAndGet()
        );
    }

    private static @NotNull String templateFolderId(
            @NotNull String cloudId,
            long folderIdSeq
    ) {
        return "%s-folder-%d".formatted(
                cloudId,
                folderIdSeq
        );
    }

    private static @NotNull String templateFolderName(@NotNull String folderId) {
        return "%s name".formatted(
                folderId
        );
    }

    private static @NotNull String templateFolderDescription(@NotNull String folderId) {
        return "%s description".formatted(
                folderId
        );
    }


    public static @NotNull Folder nextFolder(
            @NotNull Cloud cloud
    ) {
        return nextFolder(
                cloud.id()
        );
    }

    public static @NotNull Folder nextFolder(
            @NotNull String cloudId
    ) {
        return templateFolder(
                cloudId,
                nextFolderId(cloudId)
        );
    }

    private static @NotNull Folder templateFolder(
            @NotNull String cloudId,
            @NotNull String folderId
    ) {
        return new Folder(
                folderId,
                cloudId
        );
    }

    public static @NotNull PF.Folder templateProtoFolder(
            @NotNull Folder folder
    ) {
        return PF.Folder.newBuilder()
                .setId(folder.id())
                .setCloudId(folder.cloudId())
                .setName(templateFolderName(folder.id()))
                .setDescription(templateFolderDescription(folder.id()))
                .setCreatedAt(Timestamp.newBuilder()
                        .setSeconds(Instant.now().getEpochSecond())
                        .build()
                )
                .setStatus(PF.Folder.Status.ACTIVE)
                .build();
    }


    private TestResourceManagerFolders() {
    }

}
