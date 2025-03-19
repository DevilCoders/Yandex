package yandex.cloud.team.integration.abc;

import java.io.Serial;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.iam.exception.BadRequestException;
import yandex.cloud.priv.team.integration.v1.PTIAS;

/**
 * Represents an error condition occurred for an invalid {@link PTIAS.ResolveRequest}.
 */
public class InvalidResolveRequestException extends BadRequestException {

    @Serial
    private static final long serialVersionUID = 4396808988868126695L;

    private InvalidResolveRequestException(
            @NotNull String message
    ) {
        super(message);
    }

    public static @NotNull InvalidResolveRequestException empty() {
        return new InvalidResolveRequestException(
                "One of [cloud_id, abc_id, abc_slug, abc_folder_id] should be set");
    }

}
