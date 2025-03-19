package yandex.cloud.ti.yt.abcd.client;

import java.io.Serial;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.iam.exception.NotFoundException;

/**
 * Represents an client error condition for a missing ABC folder.
 */
public class AbcFolderNotFoundException extends NotFoundException {

    @Serial
    private static final long serialVersionUID = 7657542526107817242L;

    private AbcFolderNotFoundException(
        @NotNull String message
    ) {
        super(message);
    }

    public static @NotNull AbcFolderNotFoundException forAbcServiceId(
        long abcId
    ) {
        return new AbcFolderNotFoundException(
            String.format("ABC service with id %d has no default folder", abcId));
    }

    public static @NotNull AbcFolderNotFoundException forId(
        @NotNull String abcFolderId
    ) {
        return new AbcFolderNotFoundException(
            String.format("ABC folder with id '%s' does not exist", abcFolderId));
    }

}
