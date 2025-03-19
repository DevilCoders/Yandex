package yandex.cloud.team.integration.abc;

import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;
import yandex.cloud.ti.abc.AbcServiceCloud;
import yandex.cloud.ti.abc.repo.ListPage;
import yandex.cloud.ti.yt.abc.client.AbcServiceNotFoundException;

public interface ResolveServiceFacade {

    @NotNull AbcServiceCloud resolveAbcServiceCloudByAbcServiceId(
            long abcServiceId
    );

    @NotNull AbcServiceCloud resolveAbcServiceCloudByAbcServiceSlug(
            @NotNull String abcServiceSlug
    );

    @NotNull AbcServiceCloud resolveAbcServiceCloudByAbcdFolderId(
            @NotNull String abcdFolderId
    );

    default @Nullable AbcServiceCloud tryResolveAbcServiceCloudByAbcdFolderId(
            @NotNull String abcdFolderId
    ) {
        // todo refactor into other way around
        //  tryResolveByAbcdFolderId should do the work
        //  resolveByAbcdFolderId should call tryResolveByAbcdFolderId and check for null result
        try {
            return resolveAbcServiceCloudByAbcdFolderId(abcdFolderId);
        } catch (AbcServiceNotFoundException ignored) {
            return null;
        }
    }

    @NotNull AbcServiceCloud resolveAbcServiceCloudByCloudId(
            @NotNull String cloudId
    );

    @NotNull ListPage<? extends AbcServiceCloud> listAbcServiceClouds(
            long pageSize,
            @Nullable String pageToken
    );

}
