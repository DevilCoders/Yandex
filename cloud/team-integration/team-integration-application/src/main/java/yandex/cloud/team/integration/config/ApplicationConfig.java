package yandex.cloud.team.integration.config;

import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import lombok.Value;
import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;
import yandex.cloud.auth.api.AuthClientConfig;
import yandex.cloud.common.httpserver.HttpServerConfig;
import yandex.cloud.grpc.server.GrpcServerConfig;
import yandex.cloud.health.http.ManagedHealthCheckConfig;
import yandex.cloud.iam.client.tvm.config.TvmClientConfig;
import yandex.cloud.iam.config.RepositoryConfig;
import yandex.cloud.iam.config.SystemAccountServiceConfig;
import yandex.cloud.iam.grpc.remoteoperation.RemoteOperationRetryConfig;
import yandex.cloud.metrics.MetricsConfig;
import yandex.cloud.repository.db.id.ClusterConfig;
import yandex.cloud.repository.kikimr.KikimrConfig;
import yandex.cloud.task.TaskProcessorConfig;
import yandex.cloud.team.integration.abc.AbcIntegrationProperties;
import yandex.cloud.team.integration.idm.config.IamControlPlaneConfig;
import yandex.cloud.team.integration.idm.config.IdmConfig;
import yandex.cloud.ti.abcd.adapter.AbcdAdapterProperties;
import yandex.cloud.ti.billing.client.BillingPrivateConfig;
import yandex.cloud.ti.billing.client.ProductionBillingPrivateConfig;
import yandex.cloud.ti.rm.client.ResourceManagerClientProperties;
import yandex.cloud.ti.yt.abc.client.TeamAbcClientProperties;
import yandex.cloud.ti.yt.abcd.client.TeamAbcdClientProperties;
import yandex.cloud.ts.TokenServiceClientConfig;

/**
 * Application configuration.
 */
@Value
public class ApplicationConfig {

    @NotNull TeamAbcClientProperties abc;

    @Nullable AbcIntegrationProperties abcIntegration;

    @NotNull TeamAbcdClientProperties abcd;

    AbcdAdapterProperties abcdAdapter;

    @NotNull AuthClientConfig authClient;

    @JsonDeserialize(as = ProductionBillingPrivateConfig.class)
    BillingPrivateConfig billingPrivate;

    @NotNull ClusterConfig cluster;

    @NotNull RemoteOperationRetryConfig defaultRemoteOperationRetry;

    @NotNull GrpcServerConfig grpcServer;

    @NotNull HttpServerConfig httpServer;

    @NotNull TokenServiceClientConfig tokenService;

    IdmConfig idm;

    KikimrConfig kikimr;

    MetricsConfig metrics;

    ManagedHealthCheckConfig healthCheck;

    @NotNull IamControlPlaneConfig iam;

    @NotNull RepositoryConfig repository;

    @NotNull ResourceManagerClientProperties resourceManager;

    @NotNull SystemAccountServiceConfig systemAccountService;

    @NotNull TaskProcessorConfig taskProcessor;

    TvmClientConfig tvm;

    ExperimentalConfig experimental;

}
