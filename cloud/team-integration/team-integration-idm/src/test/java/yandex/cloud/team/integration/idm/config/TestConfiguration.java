package yandex.cloud.team.integration.idm.config;

import java.util.List;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.auth.SystemAccountService;
import yandex.cloud.di.Configuration;
import yandex.cloud.fake.iam.FakeClouds;
import yandex.cloud.fake.iam.FakeFolder;
import yandex.cloud.fake.iam.FakeIamServer;
import yandex.cloud.fake.iam.service.FakeSystemAccountService;
import yandex.cloud.grpc.ExceptionMapper;
import yandex.cloud.grpc.client.InProcessClientConfig;
import yandex.cloud.health.http.ManagedHealthCheckService;
import yandex.cloud.health.http.ManagedHealthCheckServiceImpl;
import yandex.cloud.iam.grpc.GrpcCallHandler;
import yandex.cloud.iam.grpc.IamExceptionMapper;
import yandex.cloud.metrics.LogMetricReporter;
import yandex.cloud.metrics.MetricReporter;
import yandex.cloud.team.integration.abc.CreateCloudServiceFacade;
import yandex.cloud.team.integration.abc.FakeResolveServiceFacade;
import yandex.cloud.team.integration.abc.ResolveServiceFacade;
import yandex.cloud.team.integration.idm.FakeCreateCloudServiceFacade;
import yandex.cloud.team.integration.idm.IdmServiceHttpServletDispatcher;
import yandex.cloud.team.integration.idm.TestResources;
import yandex.cloud.team.integration.idm.grpc.client.PassportFederationClient;
import yandex.cloud.team.integration.idm.grpc.client.PassportFederationClientImpl;
import yandex.cloud.ti.http.server.HttpServerConfiguration;
import yandex.cloud.ti.http.server.TestHttpServerConfiguration;
import yandex.cloud.ti.rm.client.MockResourceManagerClientConfiguration;

public class TestConfiguration extends Configuration {

    public static final String NAME = "IdmTest";

    @Override
    protected void configure() {
        merge(new IdmServicesConfiguration());
        put(CreateCloudServiceFacade.class, FakeCreateCloudServiceFacade::new);
        put(ExceptionMapper.class, IamExceptionMapper::new);
        put(GrpcCallHandler.class, () -> new GrpcCallHandler(get(ExceptionMapper.class)));
        merge(httpServerConfiguration());
        put(MetricReporter.class, LogMetricReporter::new);
        put(ManagedHealthCheckService.class, this::createManagedHealthCheckService);
        put(PassportFederationClient.class, this::createPassportFederationClient);
        put(ResolveServiceFacade.class, this::resolveServiceFacade);
        merge(new MockResourceManagerClientConfiguration(NAME));
        put(SystemAccountService.class, FakeSystemAccountService::new);
    }

    private @NotNull ResolveServiceFacade resolveServiceFacade() {
        FakeResolveServiceFacade.addAbcServiceCloud(
                TestResources.ABC_SERVICE_ID,
                TestResources.ABC_SERVICE_SLUG,
                "abc-folder-id-" + TestResources.ABC_SERVICE_ID,
                FakeClouds.getDefault(() -> "default-cloud-1").getId(),
                FakeFolder.DEFAULT_FOLDER.getId()
        );
        return new FakeResolveServiceFacade();
    }

    private PassportFederationClient createPassportFederationClient() {
        return new PassportFederationClientImpl(NAME,
                new InProcessClientConfig(FakeIamServer.class.getSimpleName()),
                get(SystemAccountService.class)::getIamToken);
    }

    protected @NotNull HttpServerConfiguration httpServerConfiguration() {
        return new TestHttpServerConfiguration(
                List.of(),
                List.of(
                        IdmServiceHttpServletDispatcher.class
                )
        );
    }

    private ManagedHealthCheckService createManagedHealthCheckService() {
        var managedHealthCheckService = new ManagedHealthCheckServiceImpl();
        managedHealthCheckService.enable();

        return managedHealthCheckService;
    }

}
