package yandex.cloud.team.integration;

import java.util.ArrayList;
import java.util.List;

import javax.servlet.Servlet;

import io.grpc.BindableService;
import org.jetbrains.annotations.NotNull;
import yandex.cloud.auth.SystemAccountException;
import yandex.cloud.auth.SystemAccountService;
import yandex.cloud.auth.api.AuthClientConfig;
import yandex.cloud.auth.api.CloudAuthClient;
import yandex.cloud.di.Configuration;
import yandex.cloud.fake.iam.service.FakeSystemAccountService;
import yandex.cloud.fake.iam.service.iam.PassportFederationService;
import yandex.cloud.fake.iam.service.yrm.CloudService;
import yandex.cloud.fake.iam.service.yrm.FolderService;
import yandex.cloud.fake.iam.service.yrm.OperationService;
import yandex.cloud.iam.config.DefaultSystemAccountServiceConfiguration;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.KikimrConfig;
import yandex.cloud.team.integration.application.Application;
import yandex.cloud.team.integration.config.AbstractTeamIntegrationConfiguration;
import yandex.cloud.team.integration.config.YamlConfiguration;
import yandex.cloud.team.integration.repository.TeamIntegrationInMemoryRepositoryConfiguration;
import yandex.cloud.team.integration.repository.TeamIntegrationKikimrRepositoryConfiguration;
import yandex.cloud.ti.abc.repo.ydb.AbcIntegrationEntities;
import yandex.cloud.ti.billing.FakeBillingPrivateServlet;
import yandex.cloud.ti.grpc.server.GrpcServerConfiguration;
import yandex.cloud.ti.grpc.server.TestGrpcServerConfiguration;

public class DebugConfiguration extends AbstractTeamIntegrationConfiguration {

    public static final String DEBUG_CONFIG_NAME = "/debug-application.yaml";

    @Override
    public <T> T get(Class<T> type) {
        return super.get(type);
    }

    @Override
    protected YamlConfiguration configConfiguration() {
        return new YamlConfiguration(getClass().getResource(DEBUG_CONFIG_NAME));
    }

    @Override
    protected Configuration repositoryConfiguration() {
        var config = get(KikimrConfig.class);
        @SuppressWarnings("rawtypes")
        List<List<Class<? extends Entity>>> entities = List.of(
                AbcIntegrationEntities.entities
        );
        if (config == null) {
            return new TeamIntegrationInMemoryRepositoryConfiguration(entities);
        } else {
            return new TeamIntegrationKikimrRepositoryConfiguration(entities);
        }
    }

    @Override
    protected Configuration systemAccountServiceConfiguration() {
        return new DebugSystemAccountServiceConfiguration();
    }

    @Override
    protected @NotNull List<Class<? extends Servlet>> getHttpServlets() {
        var list = new ArrayList<>(super.getHttpServlets());
        list.add(FakeBillingPrivateServlet.class);
        return List.copyOf(list);
    }

    @Override
    protected @NotNull GrpcServerConfiguration grpcServerConfiguration() {
        return new TestGrpcServerConfiguration(
                Application.NAME,
                getBindableServiceClasses()
        ) {
            @Override
            protected @NotNull BindableService[] services() {
                List<BindableService> services = new ArrayList<>(List.of(super.services()));
                var stateHolder = new IamStateHolder();
                services.add(new CloudService(stateHolder));
                services.add(new FolderService(stateHolder));
                services.add(new OperationService(stateHolder));
                services.add(new PassportFederationService(stateHolder));
                return services.toArray(BindableService[]::new);
            }
        };
    }


    @Override
    protected CloudAuthClient createCloudAuthClient() {
        if (get(AuthClientConfig.class).getPort() < 0) {
            return new DebugCloudAuthClient();
        }
        return super.createCloudAuthClient();
    }

    private static class DebugSystemAccountServiceConfiguration extends DefaultSystemAccountServiceConfiguration {

        @Override
        protected SystemAccountService makeSystemAccountService() {
            try {
                return super.makeSystemAccountService();
            } catch (SystemAccountException.NotConfiguredException ignored) {
                // Fake SA by default
                return new FakeSystemAccountService();
            }
        }

    }

}
