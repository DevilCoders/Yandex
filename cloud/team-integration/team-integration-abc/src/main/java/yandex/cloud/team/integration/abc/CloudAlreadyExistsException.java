package yandex.cloud.team.integration.abc;

import java.io.Serial;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.iam.exception.AlreadyExistsException;
import yandex.cloud.ti.abc.AbcServiceCloud;

/**
 * Represents a server error for an ABC service that already has a cloud created.
 */
public class CloudAlreadyExistsException extends AlreadyExistsException {

    @Serial
    private static final long serialVersionUID = -943251544649020598L;

    private CloudAlreadyExistsException(
            @NotNull String message
    ) {
        super(message);
    }

    public static @NotNull CloudAlreadyExistsException forService(
            @NotNull AbcServiceCloud abcService
    ) {
        return new CloudAlreadyExistsException(
                String.format("Cloud '%s' already created for ABC service id %d, slug '%s', abc folder '%s'",
                        abcService.cloudId(), abcService.abcServiceId(),
                        abcService.abcServiceSlug(), abcService.abcdFolderId()));
    }

}
