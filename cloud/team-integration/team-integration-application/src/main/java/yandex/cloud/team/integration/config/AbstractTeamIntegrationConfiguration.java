package yandex.cloud.team.integration.config;

import java.util.Collection;
import java.util.List;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import javax.servlet.Servlet;

import io.grpc.BindableService;
import org.eclipse.jetty.security.Authenticator;
import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;
import yandex.cloud.auth.SystemAccountService;
import yandex.cloud.auth.api.AuthClientConfig;
import yandex.cloud.auth.api.CloudAuthClient;
import yandex.cloud.auth.api.CloudAuthClientFactory;
import yandex.cloud.di.Configuration;
import yandex.cloud.grpc.BaseOperationConverter;
import yandex.cloud.grpc.ExceptionMapper;
import yandex.cloud.grpc.MetricsTransportFilter;
import yandex.cloud.iam.client.tvm.TvmClient;
import yandex.cloud.iam.client.tvm.config.TvmClientConfig;
import yandex.cloud.iam.config.DefaultClockConfiguration;
import yandex.cloud.iam.config.KikimrTokenClientConfiguration;
import yandex.cloud.iam.grpc.GrpcCallHandler;
import yandex.cloud.iam.grpc.IamExceptionMapper;
import yandex.cloud.metrics.MetricReporter;
import yandex.cloud.metrics.PrometheusMetricReporter;
import yandex.cloud.team.integration.abc.AbcIntegrationConfiguration;
import yandex.cloud.team.integration.abc.OperationConverter;
import yandex.cloud.team.integration.application.Application;
import yandex.cloud.team.integration.http.filter.AccessLogFilter;
import yandex.cloud.team.integration.http.filter.MetricsFilter;
import yandex.cloud.team.integration.idm.IdmServiceHttpServletDispatcher;
import yandex.cloud.team.integration.idm.config.IamControlPlaneConfig;
import yandex.cloud.team.integration.idm.config.IdmServicesConfiguration;
import yandex.cloud.team.integration.idm.grpc.client.PassportFederationClient;
import yandex.cloud.team.integration.idm.grpc.client.PassportFederationClientImpl;
import yandex.cloud.ti.abc.repo.AbcIntegrationRepositoryConfiguration;
import yandex.cloud.ti.abc.repo.ydb.YdbAbcIntegrationRepositoryConfiguration;
import yandex.cloud.ti.abcd.adapter.AbcdAdapterConfiguration;
import yandex.cloud.ti.billing.client.BillingPrivateClientConfiguration;
import yandex.cloud.ti.grpc.server.GrpcServerConfiguration;
import yandex.cloud.ti.http.server.HttpServerConfiguration;
import yandex.cloud.ti.rm.client.ResourceManagerClientConfiguration;
import yandex.cloud.ti.tvm.client.TvmClientConfiguration;
import yandex.cloud.ti.tvm.http.TvmAuthenticator;
import yandex.cloud.ti.yt.abcd.client.TeamAbcdClientConfiguration;
import yandex.cloud.ts.TokenServiceClient;
import yandex.cloud.ts.TokenServiceClientConfig;
import yandex.cloud.ts.TokenServiceClientImpl;

public abstract class AbstractTeamIntegrationConfiguration extends Configuration {

    protected abstract Configuration configConfiguration();

    protected abstract Configuration repositoryConfiguration();

    protected abstract Configuration systemAccountServiceConfiguration();

    @Override
    protected void configure() {
        merge(configConfiguration());
        merge(repositoryConfiguration());
        merge(new AbcdAdapterConfiguration(Application.NAME));
        merge(abcIntegrationConfiguration());
        merge(new DefaultClockConfiguration());
        merge(new TvmClientConfiguration());
        merge(new KikimrTokenClientConfiguration(Application.NAME));
        merge(new IdmServicesConfiguration());
        merge(systemAccountServiceConfiguration());

        put(BaseOperationConverter.class, OperationConverter::new);
        put(CloudAuthClient.class, this::createCloudAuthClient);
        put(ExceptionMapper.class, IamExceptionMapper::new);
        put(GrpcCallHandler.class, () -> new GrpcCallHandler(get(ExceptionMapper.class)));
        put(Authenticator.class, this::authenticator);
        merge(httpServerConfiguration());
        put(TokenServiceClient.class, () -> new TokenServiceClientImpl(Application.NAME, get(TokenServiceClientConfig.class)));
        merge(new BillingPrivateClientConfiguration(Application.NAME));
        put(MetricReporter.class, PrometheusMetricReporter::new);
        merge(new ResourceManagerClientConfiguration(Application.NAME));
        merge(new TeamAbcdClientConfiguration(Application.NAME));
        put(PassportFederationClient.class, this::createPassportFederationClient);
        merge(grpcServerConfiguration());
    }

    protected @NotNull AbcIntegrationConfiguration abcIntegrationConfiguration() {
        return new AbcIntegrationConfiguration(Application.NAME) {
            @Override
            protected @NotNull AbcIntegrationRepositoryConfiguration abcIntegrationRepositoryConfiguration() {
                return new YdbAbcIntegrationRepositoryConfiguration();
            }
        };
    }

    protected CloudAuthClient createCloudAuthClient() {
        return CloudAuthClientFactory.newClient(Application.NAME, get(AuthClientConfig.class));
    }

    protected @Nullable Authenticator authenticator() {
        return new TvmAuthenticator(
                get(TvmClient.class),
                get(TvmClientConfig.class)
        );
    }

    protected @NotNull HttpServerConfiguration httpServerConfiguration() {
        return new HttpServerConfiguration(
                List.of(
                        AccessLogFilter.class,
                        MetricsFilter.class
                ),
                getHttpServlets()
        );
    }

    protected @NotNull List<Class<? extends Servlet>> getHttpServlets() {
        return List.of(IdmServiceHttpServletDispatcher.class);
    }

    protected @NotNull GrpcServerConfiguration grpcServerConfiguration() {
        return new GrpcServerConfiguration(
                Application.NAME,
                getBindableServiceClasses(),
                List.of(
                        MetricsTransportFilter.getInstance()
                )
        );
    }

    protected @NotNull Collection<Class<? extends BindableService>> getBindableServiceClasses() {
        return Stream.of(
                        AbcIntegrationConfiguration.getBindableServiceClasses(),
                        AbcdAdapterConfiguration.getBindableServiceClasses()
                )
                .flatMap(Collection::stream)
                .collect(Collectors.toList());
    }


    private PassportFederationClient createPassportFederationClient() {
        return new PassportFederationClientImpl(Application.NAME,
                get(IamControlPlaneConfig.class),
                get(SystemAccountService.class)::getIamToken);
    }

}
