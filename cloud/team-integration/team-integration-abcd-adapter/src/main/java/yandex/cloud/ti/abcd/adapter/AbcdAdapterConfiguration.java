package yandex.cloud.ti.abcd.adapter;

import java.util.Collection;
import java.util.List;
import java.util.function.Supplier;

import io.grpc.BindableService;
import org.jetbrains.annotations.NotNull;
import yandex.cloud.auth.SystemAccountService;
import yandex.cloud.di.Configuration;
import yandex.cloud.iam.grpc.GrpcCallHandler;
import yandex.cloud.team.integration.abc.ResolveServiceFacade;
import yandex.cloud.ti.abcd.provider.AbcdProvider;
import yandex.cloud.ti.abcd.provider.AbcdProviderProperties;
import yandex.cloud.ti.yt.abcd.client.TeamAbcdClient;

import ru.yandex.intranet.d.backend.service.provider_proto.AccountsServiceGrpc;

public class AbcdAdapterConfiguration extends Configuration {

    private final @NotNull String applicationName;


    public AbcdAdapterConfiguration(
            @NotNull String applicationName
    ) {
        this.applicationName = applicationName;
    }


    @Override
    public void configure() {
        put(AbcdProvider[].class, this::createAbcdProvidersArray);
        put(AccountServiceFacade.class, this::createAccountServiceFacade);
        put(AccountsServiceGrpc.AccountsServiceImplBase.class, this::createAccountsServiceGrpc);

        put(AbcdProviderRegistry.class, () -> new AbcdProviderRegistry(
                List.of(get(AbcdProvider[].class))
        ));
    }


    protected @NotNull AbcdProvider[] createAbcdProvidersArray() {
        Supplier<String> tokenSupplier = get(SystemAccountService.class)::getIamToken;
        AbcdAdapterProperties abcdAdapterProperties = get(AbcdAdapterProperties.class);
        // todo ensure that AbcdAdapterProperties is not null, remove the check below
        if (abcdAdapterProperties == null) {
            return new AbcdProvider[0];
        }

        List<AbcdProvider> abcdProviders = createAbcdProviders(
                applicationName,
                tokenSupplier,
                abcdAdapterProperties.providers()
        );
        return abcdProviders.toArray(new AbcdProvider[0]);
    }

    private static @NotNull List<AbcdProvider> createAbcdProviders(
            @NotNull String applicationName,
            @NotNull Supplier<String> tokenSupplier,
            @NotNull Collection<AbcdProviderProperties> abcdProviderPropertiesList
    ) {
        return abcdProviderPropertiesList.stream()
                .map(abcdProviderProperties -> createAbcdProvider(
                        applicationName,
                        tokenSupplier,
                        abcdProviderProperties
                ))
                .toList();
    }

    private static @NotNull AbcdProvider createAbcdProvider(
            @NotNull String applicationName,
            @NotNull Supplier<String> tokenSupplier,
            @NotNull AbcdProviderProperties abcdProviderProperties
    ) {
        return new AbcdProvider(
                abcdProviderProperties.id(),
                abcdProviderProperties.name(),
                CloudQuotaServiceClientFactories
                        .getFactoryByQuotaServiceName(abcdProviderProperties.quotaService().name())
                        .createCloudQuotaServiceClient(
                                applicationName,
                                abcdProviderProperties.quotaService().endpoint(),
                                tokenSupplier
                        ),
                abcdProviderProperties.quotaQueryThreads(),
                abcdProviderProperties.mappedQuotaMetrics()
        );
    }

    protected @NotNull AccountServiceFacade createAccountServiceFacade() {
        return new AccountServiceFacadeImpl(
                get(AbcdProviderRegistry.class),
                get(ResolveServiceFacade.class),
                get(TeamAbcdClient.class)
        );
    }

    protected @NotNull AccountsServiceGrpc.AccountsServiceImplBase createAccountsServiceGrpc() {
        return new AccountsServiceGrpcImpl(
                get(AccountServiceFacade.class),
                get(GrpcCallHandler.class)
        );
    }

    public static @NotNull Collection<Class<? extends BindableService>> getBindableServiceClasses() {
        return List.of(
                AccountsServiceGrpc.AccountsServiceImplBase.class
        );
    }

}
