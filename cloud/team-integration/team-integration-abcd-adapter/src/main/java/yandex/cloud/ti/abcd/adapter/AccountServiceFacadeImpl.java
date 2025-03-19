package yandex.cloud.ti.abcd.adapter;

import java.util.List;
import java.util.concurrent.CompletableFuture;

import lombok.extern.log4j.Log4j2;
import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;
import yandex.cloud.team.integration.abc.ResolveServiceFacade;
import yandex.cloud.ti.abc.AbcServiceCloud;
import yandex.cloud.ti.abc.repo.ListPage;
import yandex.cloud.ti.abcd.provider.AbcdProvider;
import yandex.cloud.ti.yt.abc.client.AbcServiceNotFoundException;
import yandex.cloud.ti.yt.abcd.client.TeamAbcdClient;
import yandex.cloud.ti.yt.abcd.client.TeamAbcdFolder;

@Log4j2
class AccountServiceFacadeImpl implements AccountServiceFacade {

    private static final String ABC_SERVICE_ID = "abc_service_id";
    private static final String FOLDER_ID = "folder_id";


    private final @NotNull AbcdProviderRegistry providerRegistry;
    private final @NotNull ResolveServiceFacade resolveServiceFacade;
    private final @NotNull TeamAbcdClient teamAbcdClient;

    private final int listAccountsMaxPageSize;


    AccountServiceFacadeImpl(
            @NotNull AbcdProviderRegistry providerRegistry,
            @NotNull ResolveServiceFacade resolveServiceFacade,
            @NotNull TeamAbcdClient teamAbcdClient
    ) {
        this.providerRegistry = providerRegistry;
        this.resolveServiceFacade = resolveServiceFacade;
        this.teamAbcdClient = teamAbcdClient;

        // todo CLOUD-82142
        //  move max page size into config
        listAccountsMaxPageSize = 100;
    }


    @Override
    public @NotNull AbcdAccount createAccount(
            @NotNull CreateAccountParameters parameters
    ) {
        log.debug("create abcd account for provider {}, abc service {}, abc folder {}",
                parameters.getProviderId(), parameters.getAbcServiceId(), parameters.getAbcFolderId()
        );

        AbcdProvider provider = providerRegistry.getProvider(parameters.getProviderId());

        TeamAbcdFolder abcdFolder = teamAbcdClient.getAbcdFolder(parameters.getAbcFolderId());
        if (abcdFolder.abcServiceId() != parameters.getAbcServiceId()) {
            // todo provide sane exception message and details
            throw new InvalidCreateAccountRequestException("abc service id mismatch");
        }
        if (!abcdFolder.defaultForService()) {
            // todo provide sane exception message and details
            // todo decide between INVALID_ARGUMENT and UNIMPLEMENTED
            throw new InvalidCreateAccountRequestException("creating abcd account for non-default abc folder is not supported (abc_service_id=%s, abc_folder_id=%s)".formatted(
                    parameters.getAbcServiceId(), parameters.getAbcFolderId()
            ));
        }

        AbcServiceCloud abcServiceCloud = resolveServiceFacade.tryResolveAbcServiceCloudByAbcdFolderId(parameters.getAbcFolderId());
        if (abcServiceCloud == null) {
            // todo try to create a cloud
//            CreateCloudServiceFacade createCloudServiceFacade = null;
//            Operation byAbcFolderId = createCloudServiceFacade.createByAbcdFolderId(parameters.abcFolderId());
            // todo handle already exists, re-resolve
//            if (byAbcFolderId.isDone()) {
//                OperationResponse response = byAbcFolderId.getResponse();
//                // todo response should be CreateCloudResponse?
//            }

            // todo provide sane exception message and details
            // todo provide retry info
            throw new AbcdAccountNotReadyException("cloud not ready (abc_service_id=%s, abc_folder_id=%s)".formatted(
                    parameters.getAbcServiceId(), parameters.getAbcFolderId()
            ));
        }
        checkMatch(abcServiceCloud, parameters.getAbcServiceId(), parameters.getAbcFolderId());

        // todo proper exception message
        //  also add trailers with google.rpc.ResourceInfo
        // todo also check that abcServiceId matches?
        throw new AbcdAccountAlreadyExistsException("abcd account already exists request=(%s, %s), exists=(%s, %s, %s)".formatted(
                parameters.getAbcServiceId(), parameters.getAbcFolderId(),
                abcServiceCloud.abcServiceId(), abcServiceCloud.abcdFolderId(), abcServiceCloud.cloudId()
        ));
    }

    @Override
    public @NotNull AbcdAccount getAccount(
            @NotNull GetAccountParameters parameters
    ) {
        AbcdProvider provider = providerRegistry.getProvider(parameters.getProviderId());
        AbcServiceCloud abcServiceCloud = getAccountEntity(
                parameters.getAccountId(),
                parameters.getAbcServiceId(),
                parameters.getAbcFolderId()
        );

        return toAbcdAccount(abcServiceCloud, provider, parameters.isWithProvisions());
    }

    @Override
    public @NotNull ListPage<AbcdAccount> listAccounts(
            @NotNull ListAccountsParameters parameters
    ) {
        AbcdProvider provider = providerRegistry.getProvider(parameters.getProviderId());
        if (parameters.getAbcFolderId() != null) {
            AbcServiceCloud abcServiceCloud = resolveServiceFacade.tryResolveAbcServiceCloudByAbcdFolderId(parameters.getAbcFolderId());
            if (abcServiceCloud == null) {
                return new ListPage<>(
                        List.of(),
                        null
                );
            } else {
                return new ListPage<>(
                        List.of(toAbcdAccount(
                                abcServiceCloud,
                                provider,
                                parameters.isWithProvisions()
                        )),
                        null
                );
            }
        } else {
            long limit = Math.min(listAccountsMaxPageSize, parameters.getLimit());
            ListPage<? extends AbcServiceCloud> abcServiceClouds = resolveServiceFacade.listAbcServiceClouds(
                    limit,
                    parameters.getPageToken()
            );
            ListPage<AbcdAccount> abcdAccounts = abcServiceClouds.map(it -> toAbcdAccount(it, provider, false));
            if (parameters.isWithProvisions()) {
                abcdAccounts = new ListPage<>(
                        withProvisions(provider, abcdAccounts.items()),
                        abcdAccounts.nextPageToken()
                );
            }
            return abcdAccounts;
        }
    }

    @Override
    public @NotNull AbcdAccount updateProvision(
            @NotNull UpdateProvisionParameters parameters
    ) {
        log.debug("update provision for abcd account {} for provider {}, abc service {}, abc folder {}",
                parameters.accountId(),
                parameters.providerId(), parameters.abcServiceId(), parameters.abcFolderId()
        );
        AbcdProvider provider = providerRegistry.getProvider(parameters.providerId());
        AbcServiceCloud abcServiceCloud = getAccountEntity(
                parameters.accountId(),
                parameters.abcServiceId(),
                parameters.abcFolderId()
        );

        provider.updateQuotaMetrics(abcServiceCloud.cloudId(), parameters.mappedQuotaMetricUpdates());

        // todo return only those quotes/provisions that were changed
        return toAbcdAccount(abcServiceCloud, provider, true);
    }


    private @NotNull AbcServiceCloud getAccountEntity(
            @NotNull String accountId,
            long abcServiceId, @NotNull String abcFolderId
    ) {
        AbcServiceCloud abcServiceCloud;
        try {
            abcServiceCloud = resolveServiceFacade.resolveAbcServiceCloudByCloudId(accountId);
        } catch (AbcServiceNotFoundException ignored) {
            throw AbcdAccountNotFoundException.of(accountId);
        }
        checkMatch(abcServiceCloud, abcServiceId, abcFolderId);
        return abcServiceCloud;
    }

    private void checkMatch(
            @NotNull AbcServiceCloud abcServiceCloud,
            long abcServiceId,
            @NotNull String abcFolderId
    ) {
        // todo throw INVALID_ARGUMENT from these checks
        Validator.checkMatch(abcServiceCloud.abcServiceId(), abcServiceId, ABC_SERVICE_ID);
        Validator.checkMatch(abcServiceCloud.abcdFolderId(), abcFolderId, FOLDER_ID);
    }

    private @NotNull AbcdAccount toAbcdAccount(
            @NotNull AbcServiceCloud abcServiceCloud,
            @NotNull AbcdProvider provider,
            boolean withProvisions
    ) {
        return new AbcdAccount(
                abcServiceCloud.cloudId(),
                provider.getId(),
                abcServiceCloud.abcServiceId(),
                abcServiceCloud.abcdFolderId(),
                generateAbcdAccountName(abcServiceCloud, provider),
                withProvisions
                        ? provider.getQuotaMetrics(abcServiceCloud.cloudId())
                        : List.of()
        );
    }

    private @NotNull List<? extends AbcdAccount> withProvisions(
            @NotNull AbcdProvider provider,
            @NotNull List<? extends AbcdAccount> abcdAccounts
    ) {
        List<CompletableFuture<AbcdAccount>> futures = abcdAccounts.stream()
                .map(abcdAccount -> CompletableFuture.supplyAsync(
                        () -> abcdAccount.withProvisions(provider.getQuotaMetrics(abcdAccount.id())),
                        provider.getQuotaRequestExecutor()
                ))
                .toList();
        // create all futures before joining on any of them
        return futures.stream()
                .map(CompletableFuture::join)
                .toList();
    }

    private @Nullable String generateAbcdAccountName(
            @NotNull AbcServiceCloud abcServiceCloud,
            @NotNull AbcdProvider provider
    ) {
        // todo abc-slug/abcd-folder-name/abcd-provider-name?
        return "%s/%s".formatted(
                abcServiceCloud.abcServiceSlug(),
                provider.getName()
        );
    }

}
