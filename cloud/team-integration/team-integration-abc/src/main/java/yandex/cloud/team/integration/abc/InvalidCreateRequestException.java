package yandex.cloud.team.integration.abc;

import java.io.Serial;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.iam.exception.BadRequestException;
import yandex.cloud.priv.team.integration.v1.PTIAS;

/**
 * Represents an error condition occurred for an invalid {@link PTIAS.CreateCloudRequest}.
 */
public class InvalidCreateRequestException extends BadRequestException {

    @Serial
    private static final long serialVersionUID = 1376498837836478295L;

    private InvalidCreateRequestException(
            @NotNull String message
    ) {
        super(message);
    }

    public static @NotNull InvalidCreateRequestException empty() {
        return new InvalidCreateRequestException("One of [abc_id, abc_slug, abc_folder_id] should be set");
    }

    public static @NotNull InvalidCreateRequestException parameterMismatch() {
        return new InvalidCreateRequestException(
                "The set of parameters for requests with the same idempotence key must match");
    }

    public static @NotNull InvalidCreateRequestException nonDefaultFolder(
            @NotNull String abcFolderId,
            long abcServiceId
    ) {
        return new InvalidCreateRequestException(
                String.format("The folder '%s' is not default for the service %d",
                        abcFolderId, abcServiceId));
    }

}
