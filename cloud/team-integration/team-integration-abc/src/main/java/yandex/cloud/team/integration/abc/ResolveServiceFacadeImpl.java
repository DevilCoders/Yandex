package yandex.cloud.team.integration.abc;

import java.util.Optional;

import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;
import yandex.cloud.iam.service.TransactionHandler;
import yandex.cloud.ti.abc.AbcServiceCloud;
import yandex.cloud.ti.abc.repo.AbcIntegrationRepository;
import yandex.cloud.ti.abc.repo.ListPage;
import yandex.cloud.ti.yt.abc.client.AbcServiceNotFoundException;

public class ResolveServiceFacadeImpl implements ResolveServiceFacade {

    private final @NotNull AbcIntegrationRepository abcIntegrationRepository;


    public ResolveServiceFacadeImpl(
            @NotNull AbcIntegrationRepository abcIntegrationRepository
    ) {
        this.abcIntegrationRepository = abcIntegrationRepository;
        // todo create caches, preferably of Caffeine style, for resolveAbcServiceCloudByXXX methods
    }


    @Override
    public @NotNull AbcServiceCloud resolveAbcServiceCloudByAbcServiceId(long abcServiceId) {
        return loadByAbcServiceId(abcServiceId);
    }

    @Override
    public @NotNull AbcServiceCloud resolveAbcServiceCloudByAbcServiceSlug(@NotNull String abcServiceSlug) {
        return loadByAbcServiceSlug(abcServiceSlug);
    }

    @Override
    public @NotNull AbcServiceCloud resolveAbcServiceCloudByAbcdFolderId(@NotNull String abcdFolderId) {
        return loadByAbcdFolderId(abcdFolderId);
    }

    @Override
    public @NotNull AbcServiceCloud resolveAbcServiceCloudByCloudId(@NotNull String cloudId) {
        return loadByCloudId(cloudId);
    }

    @Override
    public @NotNull ListPage<? extends AbcServiceCloud> listAbcServiceClouds(
            long pageSize,
            @Nullable String pageToken
    ) {
        return TransactionHandler.runInReadOnlyTx(() ->
                abcIntegrationRepository.listAbcServiceClouds(pageSize, pageToken)
        );
    }


    private @NotNull AbcServiceCloud loadByAbcServiceId(
            long abcServiceId
    ) {
        return Optional
                .ofNullable(TransactionHandler.runInReadOnlyTx(() ->
                        abcIntegrationRepository.findAbcServiceCloudByAbcServiceId(abcServiceId)
                ))
                .orElseThrow(() -> AbcServiceNotFoundException.forAbcId(abcServiceId));
    }

    private @NotNull AbcServiceCloud loadByAbcServiceSlug(
            @NotNull String abcServiceSlug
    ) {
        return Optional
                .ofNullable(TransactionHandler.runInReadOnlyTx(() ->
                        abcIntegrationRepository.findAbcServiceCloudByAbcServiceSlug(abcServiceSlug)
                ))
                .orElseThrow(() -> AbcServiceNotFoundException.forAbcSlug(abcServiceSlug));
    }

    private @NotNull AbcServiceCloud loadByAbcdFolderId(
            @NotNull String abcdFolderId
    ) {
        return Optional
                .ofNullable(TransactionHandler.runInReadOnlyTx(() ->
                        abcIntegrationRepository.findAbcServiceCloudByAbcdFolderId(abcdFolderId)
                ))
                .orElseThrow(() -> AbcServiceNotFoundException.forAbcFolderId(abcdFolderId));
    }

    private @NotNull AbcServiceCloud loadByCloudId(
            @NotNull String cloudId
    ) {
        return Optional
                .ofNullable(TransactionHandler.runInReadOnlyTx(() ->
                        abcIntegrationRepository.findAbcServiceCloudByCloudId(cloudId)
                ))
                .orElseThrow(() -> AbcServiceNotFoundException.forCloud(cloudId));
    }

}
