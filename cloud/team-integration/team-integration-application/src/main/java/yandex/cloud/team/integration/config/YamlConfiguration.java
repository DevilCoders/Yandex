package yandex.cloud.team.integration.config;

import java.net.URL;

import lombok.AllArgsConstructor;
import yandex.cloud.auth.api.AuthClientConfig;
import yandex.cloud.common.httpserver.HttpServerConfig;
import yandex.cloud.config.ConfigLoader;
import yandex.cloud.di.Configuration;
import yandex.cloud.grpc.server.GrpcServerConfig;
import yandex.cloud.health.http.ManagedHealthCheckConfig;
import yandex.cloud.iam.config.RepositoryConfig;
import yandex.cloud.iam.config.SystemAccountServiceConfig;
import yandex.cloud.iam.grpc.remoteoperation.RemoteOperationRetryConfig;
import yandex.cloud.metrics.MetricsConfig;
import yandex.cloud.repository.db.id.ClusterConfig;
import yandex.cloud.repository.kikimr.KikimrConfig;
import yandex.cloud.task.TaskProcessorConfig;
import yandex.cloud.team.integration.abc.AbcIntegrationPropertiesConfiguration;
import yandex.cloud.team.integration.idm.config.IamControlPlaneConfig;
import yandex.cloud.team.integration.idm.config.IdmConfig;
import yandex.cloud.ti.abcd.adapter.AbcdAdapterPropertiesConfiguration;
import yandex.cloud.ti.billing.client.BillingPrivateConfig;
import yandex.cloud.ti.rm.client.ResourceManagerClientPropertiesConfiguration;
import yandex.cloud.ti.tvm.client.TvmClientPropertiesConfiguration;
import yandex.cloud.ti.yt.abc.client.TeamAbcClientPropertiesConfiguration;
import yandex.cloud.ti.yt.abcd.client.TeamAbcdClientPropertiesConfiguration;
import yandex.cloud.ts.TokenServiceClientConfig;

/**
 * A YAML file based configuration.
 */
@AllArgsConstructor
public class YamlConfiguration extends Configuration {

    private final URL configUrl;

    @Override
    protected void configure() {
        put(ApplicationConfig.class, () -> ConfigLoader.loadConfig(ApplicationConfig.class, configUrl));
        merge(new TeamAbcClientPropertiesConfiguration(
                get(ApplicationConfig.class)::getAbc
        ));
        merge(new TeamAbcdClientPropertiesConfiguration(
                get(ApplicationConfig.class)::getAbcd
        ));
        merge(new AbcdAdapterPropertiesConfiguration(
                get(ApplicationConfig.class)::getAbcdAdapter
        ));
        put(AuthClientConfig.class, get(ApplicationConfig.class)::getAuthClient);
        put(BillingPrivateConfig.class, get(ApplicationConfig.class)::getBillingPrivate);
        put(ClusterConfig.class, get(ApplicationConfig.class)::getCluster);
        put(GrpcServerConfig.class, get(ApplicationConfig.class)::getGrpcServer);
        put(HttpServerConfig.class, get(ApplicationConfig.class)::getHttpServer);
        put(TokenServiceClientConfig.class, get(ApplicationConfig.class)::getTokenService);
        put(IdmConfig.class, get(ApplicationConfig.class)::getIdm);
        put(KikimrConfig.class, get(ApplicationConfig.class)::getKikimr);
        put(MetricsConfig.class, get(ApplicationConfig.class)::getMetrics);
        put(ManagedHealthCheckConfig.class, get(ApplicationConfig.class)::getHealthCheck);
        put(IamControlPlaneConfig.class, get(ApplicationConfig.class)::getIam);
        put(RemoteOperationRetryConfig.class, get(ApplicationConfig.class)::getDefaultRemoteOperationRetry);
        put(RepositoryConfig.class, get(ApplicationConfig.class)::getRepository);
        merge(new ResourceManagerClientPropertiesConfiguration(
                get(ApplicationConfig.class)::getResourceManager
        ));
        merge(new AbcIntegrationPropertiesConfiguration(
                get(ApplicationConfig.class)::getAbcIntegration
        ));
        put(SystemAccountServiceConfig.class, get(ApplicationConfig.class)::getSystemAccountService);
        put(TaskProcessorConfig.class, get(ApplicationConfig.class)::getTaskProcessor);
        merge(new TvmClientPropertiesConfiguration(
                get(ApplicationConfig.class)::getTvm
        ));
        put(ExperimentalConfig.class, get(ApplicationConfig.class)::getExperimental);
    }

}
