package yandex.cloud.team.integration.abc;

import java.time.Duration;
import java.util.List;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.auth.SystemAccountService;
import yandex.cloud.auth.api.CloudAuthClient;
import yandex.cloud.auth.api.FakeCloudAuthClient;
import yandex.cloud.auth.api.Subject;
import yandex.cloud.auth.api.credentials.AbstractCredentials;
import yandex.cloud.common.httpserver.HttpServer;
import yandex.cloud.di.Configuration;
import yandex.cloud.fake.iam.service.FakeSystemAccountService;
import yandex.cloud.grpc.BaseOperationConverter;
import yandex.cloud.grpc.ExceptionMapper;
import yandex.cloud.iam.config.DefaultTaskProcessorConfiguration;
import yandex.cloud.iam.grpc.GrpcCallHandler;
import yandex.cloud.iam.grpc.IamExceptionMapper;
import yandex.cloud.iam.grpc.remoteoperation.RemoteOperationRetryConfig;
import yandex.cloud.iam.repository.tracing.WorkerTracingTaskProcessorThreadFactories;
import yandex.cloud.metrics.LogMetricReporter;
import yandex.cloud.metrics.MetricReporter;
import yandex.cloud.task.TaskProcessorConfig;
import yandex.cloud.task.TaskProcessorThreadFactories;
import yandex.cloud.ti.abc.repo.AbcIntegrationRepositoryConfiguration;
import yandex.cloud.ti.abc.repo.InMemoryAbcIntegrationRepositoryConfiguration;
import yandex.cloud.ti.billing.FakeBillingPrivateServlet;
import yandex.cloud.ti.billing.client.BillingPrivateClientConfiguration;
import yandex.cloud.ti.billing.client.BillingPrivateConfig;
import yandex.cloud.ti.billing.client.TestBillingPrivateConfig;
import yandex.cloud.ti.grpc.server.GrpcServerConfiguration;
import yandex.cloud.ti.grpc.server.TestGrpcServerConfiguration;
import yandex.cloud.ti.http.server.HttpServerConfiguration;
import yandex.cloud.ti.http.server.TestHttpServerConfiguration;
import yandex.cloud.ti.rm.client.MockResourceManagerClientConfiguration;
import yandex.cloud.ti.tvm.client.TestTvmClientConfiguration;
import yandex.cloud.ti.yt.abc.client.MockTeamAbcClientConfiguration;
import yandex.cloud.ti.yt.abc.client.TeamAbcClientConfiguration;
import yandex.cloud.ti.yt.abcd.client.MockTeamAbcdClientConfiguration;

public class TestAbcIntegrationConfiguration extends AbcIntegrationConfiguration {

    public static final String NAME = "AbcTest";

    public TestAbcIntegrationConfiguration() {
        super(NAME);
    }

    @Override
    protected void configure() {
        super.configure();
        merge(new MockTeamAbcdClientConfiguration(NAME));
        put(BaseOperationConverter.class, OperationConverter::new);
        merge(new BillingPrivateClientConfiguration(NAME));
        put(BillingPrivateConfig.class, () -> new TestBillingPrivateConfig(get(HttpServer.class)));
        put(CloudAuthClient.class, this::createCloudAuthClient);
        put(ExceptionMapper.class, IamExceptionMapper::new);
        put(GrpcCallHandler.class, () -> new GrpcCallHandler(get(ExceptionMapper.class)));
        merge(httpServerConfiguration());
        put(MetricReporter.class, LogMetricReporter::new);
        put(RemoteOperationRetryConfig.class, () -> new RemoteOperationRetryConfig(Duration.ZERO));
        merge(new MockResourceManagerClientConfiguration(NAME));
        merge(new TestTvmClientConfiguration());
        merge(grpcServerConfiguration());
        put(SystemAccountService.class, FakeSystemAccountService::new);
        put(TaskProcessorConfig.class, () -> TaskProcessorConfig.createDefault(1));
        put(AbcIntegrationProperties.class, () -> new AbcIntegrationProperties(
                "abcServiceCloudOrganizationId"
        ));
    }

    @Override
    protected TeamAbcClientConfiguration teamAbcClientConfiguration() {
        return new MockTeamAbcClientConfiguration(NAME);
    }

    @Override
    protected @NotNull AbcIntegrationRepositoryConfiguration abcIntegrationRepositoryConfiguration() {
        return new InMemoryAbcIntegrationRepositoryConfiguration();
    }

    protected @NotNull GrpcServerConfiguration grpcServerConfiguration() {
        return new TestGrpcServerConfiguration(
                NAME,
                AbcIntegrationConfiguration.getBindableServiceClasses()
        );
    }

    @Override
    protected @NotNull Configuration taskProcessorConfiguration() {
        return new DefaultTaskProcessorConfiguration() {
            @Override
            protected TaskProcessorThreadFactories getThreadFactories() {
                return new WorkerTracingTaskProcessorThreadFactories(get(TaskProcessorConfig.class));
            }
        };
    }

    private CloudAuthClient createCloudAuthClient() {
        return new FakeCloudAuthClient() {

            @Override
            public Subject authenticate(AbstractCredentials credentials) {
                return Subject.Anonymous::id;
            }

        };
    }

    protected @NotNull HttpServerConfiguration httpServerConfiguration() {
        return new TestHttpServerConfiguration(
                List.of(),
                List.of(
                        FakeBillingPrivateServlet.class
                )
        );
    }

}
