package yandex.cloud.ti.yt.abc.client;

import java.io.Serial;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.iam.exception.NotFoundException;

/**
 * Represents an client error condition for an ABC service that is not found (404 for HTTP).
 */
public class AbcServiceNotFoundException extends NotFoundException {

    @Serial
    private static final long serialVersionUID = -9049805410857551232L;

    private AbcServiceNotFoundException(
        @NotNull String message
    ) {
        super(message);
    }

    public static @NotNull AbcServiceNotFoundException forAbcId(
        long abcId
    ) {
        return new AbcServiceNotFoundException(
            String.format("ABC service for id %d not found", abcId));
    }

    public static @NotNull AbcServiceNotFoundException forAbcSlug(
        @NotNull String abcSlug
    ) {
        return new AbcServiceNotFoundException(
            String.format("ABC service for slug '%s' not found", abcSlug));
    }

    public static @NotNull AbcServiceNotFoundException forAbcFolderId(
        @NotNull String abcFolderId
    ) {
        return new AbcServiceNotFoundException(
            String.format("ABC service for folder id '%s' not found", abcFolderId));
    }

    public static @NotNull AbcServiceNotFoundException forCloud(
        @NotNull String cloudId
    ) {
        return new AbcServiceNotFoundException(
            String.format("ABC service for cloud '%s' not found", cloudId));
    }

}
