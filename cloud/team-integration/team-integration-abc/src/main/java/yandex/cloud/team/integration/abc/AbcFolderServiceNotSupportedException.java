package yandex.cloud.team.integration.abc;

import java.io.Serial;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.iam.exception.UnsupportedException;

/**
 * Represents an client error condition for a disables ABC folder service.
 */
public class AbcFolderServiceNotSupportedException extends UnsupportedException {

    @Serial
    private static final long serialVersionUID = -8248870033636025634L;

    private AbcFolderServiceNotSupportedException(
            @NotNull String message
    ) {
        super(message);
    }

    public static @NotNull AbcFolderServiceNotSupportedException of() {
        return new AbcFolderServiceNotSupportedException("ABC FolderService not configured");
    }

}
