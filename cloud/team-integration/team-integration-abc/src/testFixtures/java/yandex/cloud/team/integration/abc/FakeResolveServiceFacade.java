package yandex.cloud.team.integration.abc;

import java.util.ArrayList;
import java.util.List;

import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;
import yandex.cloud.ti.abc.AbcServiceCloud;
import yandex.cloud.ti.abc.repo.ListPage;
import yandex.cloud.ti.yt.abc.client.AbcServiceNotFoundException;

public class FakeResolveServiceFacade implements ResolveServiceFacade {

    private static final @NotNull List<AbcServiceCloud> abcServiceClouds = new ArrayList<>();


    public static void clearAbcServiceClouds() {
        abcServiceClouds.clear();
    }

    public static void addAbcServiceCloud(@NotNull AbcServiceCloud abcServiceCloud) {
        abcServiceClouds.add(abcServiceCloud);
    }

    public static void addAbcServiceCloud(
            long abcServiceId,
            @NotNull String abcServiceSlug,
            @NotNull String abcFolderId,
            @NotNull String cloudId,
            @NotNull String folderId
    ) {
        addAbcServiceCloud(new AbcServiceCloud(
                abcServiceId,
                abcServiceSlug,
                abcFolderId,
                cloudId,
                folderId
        ));
    }


    @Override
    public @NotNull AbcServiceCloud resolveAbcServiceCloudByAbcServiceId(long abcServiceId) {
        return abcServiceClouds.stream()
                .filter(it -> it.abcServiceId() == abcServiceId)
                .findFirst()
                .orElseThrow(() -> AbcServiceNotFoundException.forAbcId(abcServiceId));
    }

    @Override
    public @NotNull AbcServiceCloud resolveAbcServiceCloudByAbcServiceSlug(@NotNull String abcServiceSlug) {
        return abcServiceClouds.stream()
                .filter(it -> it.abcServiceSlug().equals(abcServiceSlug))
                .findFirst()
                .orElseThrow(() -> AbcServiceNotFoundException.forAbcSlug(abcServiceSlug));
    }

    @Override
    public @NotNull AbcServiceCloud resolveAbcServiceCloudByAbcdFolderId(@NotNull String abcdFolderId) {
        return abcServiceClouds.stream()
                .filter(it -> it.abcdFolderId().equals(abcdFolderId))
                .findFirst()
                .orElseThrow(() -> AbcServiceNotFoundException.forAbcFolderId(abcdFolderId));
    }

    @Override
    public @NotNull AbcServiceCloud resolveAbcServiceCloudByCloudId(@NotNull String cloudId) {
        return abcServiceClouds.stream()
                .filter(it -> it.cloudId().equals(cloudId))
                .findFirst()
                .orElseThrow(() -> AbcServiceNotFoundException.forCloud(cloudId));
    }

    @Override
    public @NotNull ListPage<? extends AbcServiceCloud> listAbcServiceClouds(
            long pageSize,
            @Nullable String pageToken
    ) {
        if (pageSize < 1 || pageSize > 1000) {
            throw new IllegalArgumentException();
        }
        return new ListPage<>(
                abcServiceClouds,
                null
        );
    }

}
