package yandex.cloud.team.integration.abc;

import java.util.Collection;
import java.util.List;

import io.grpc.BindableService;
import org.jetbrains.annotations.NotNull;
import yandex.cloud.di.Configuration;
import yandex.cloud.grpc.BaseOperationConverter;
import yandex.cloud.iam.config.DefaultTaskProcessorConfiguration;
import yandex.cloud.iam.grpc.GrpcCallHandler;
import yandex.cloud.iam.operation.OperationExceptionInfo;
import yandex.cloud.priv.team.integration.v1.AbcServiceGrpc;
import yandex.cloud.priv.team.integration.v1.OperationServiceGrpc;
import yandex.cloud.task.TaskProcessor;
import yandex.cloud.ti.abc.repo.AbcIntegrationRepository;
import yandex.cloud.ti.abc.repo.AbcIntegrationRepositoryConfiguration;
import yandex.cloud.ti.operation.OperationConfiguration;
import yandex.cloud.ti.operation.OperationService;
import yandex.cloud.ti.yt.abc.client.TeamAbcClient;
import yandex.cloud.ti.yt.abc.client.TeamAbcClientConfiguration;
import yandex.cloud.ti.yt.abcd.client.TeamAbcdClient;
import yandex.cloud.util.Json;

public abstract class AbcIntegrationConfiguration extends Configuration {

    static {
        Json.registerSubtype(CreateCloudJob.class, "CreateCloudJob");
        Json.registerSubtype(CreateCloudMetadata.class, "CreateCloudMetadata");
        Json.registerSubtype(CreateCloudResponse.class, "CreateCloudResponse");
        Json.registerSubtype(OperationExceptionInfo.class, "OperationExceptionInfo");
    }


    private final @NotNull String applicationName;


    public AbcIntegrationConfiguration(
            @NotNull String applicationName
    ) {
        this.applicationName = applicationName;
    }


    @Override
    protected void configure() {
        put(CreateCloudServiceFacade.class, this::createCloudServiceFacade);
        put(NamePolicyService.class, this::namePolicyService);
        put(OperationServiceFacade.class, this::operationServiceFacade);
        put(ResolveServiceFacade.class, this::resolveServiceFacade);
        put(StubOperationService.class, this::stubOperationService);
        put(AbcServiceGrpc.AbcServiceImplBase.class, this::abcServiceGrpc);
        put(OperationServiceGrpc.OperationServiceImplBase.class, this::operationServiceGrpc);
        merge(teamAbcClientConfiguration());
        merge(abcIntegrationRepositoryConfiguration());
        merge(operationConfiguration());
        merge(taskProcessorConfiguration());
    }


    protected CreateCloudServiceFacade createCloudServiceFacade() {
        return new CreateCloudServiceFacadeImpl(
                get(AbcIntegrationRepository.class),
                get(StubOperationService.class),
                get(TeamAbcClient.class),
                get(TeamAbcdClient.class),
                get(TaskProcessor.class)
        );
    }

    protected NamePolicyService namePolicyService() {
        return new NamePolicyServiceImpl();
    }

    protected OperationServiceFacade operationServiceFacade() {
        return new OperationServiceFacadeImpl(
                get(AbcIntegrationRepository.class),
                get(StubOperationService.class),
                get(OperationService.class)
        );
    }

    protected ResolveServiceFacade resolveServiceFacade() {
        return new ResolveServiceFacadeImpl(
                get(AbcIntegrationRepository.class)
        );
    }

    protected StubOperationService stubOperationService() {
        return new StubOperationServiceImpl(
                get(AbcIntegrationRepository.class),
                get(OperationService.class)
        );
    }

    protected AbcServiceGrpc.AbcServiceImplBase abcServiceGrpc() {
        return new AbcServiceGrpcImpl(
                get(CreateCloudServiceFacade.class),
                get(ResolveServiceFacade.class),
                get(BaseOperationConverter.class),
                get(GrpcCallHandler.class)
        );
    }

    protected OperationServiceGrpc.OperationServiceImplBase operationServiceGrpc() {
        return new OperationServiceGrpcImpl(
                get(GrpcCallHandler.class),
                get(BaseOperationConverter.class),
                get(OperationServiceFacade.class)
        );
    }

    protected TeamAbcClientConfiguration teamAbcClientConfiguration() {
        return new TeamAbcClientConfiguration(applicationName);
    }

    protected abstract @NotNull AbcIntegrationRepositoryConfiguration abcIntegrationRepositoryConfiguration();

    public static @NotNull Collection<Class<? extends BindableService>> getBindableServiceClasses() {
        return List.of(
                AbcServiceGrpc.AbcServiceImplBase.class,
                OperationServiceGrpc.OperationServiceImplBase.class
        );
    }

    protected @NotNull OperationConfiguration operationConfiguration() {
        return new OperationConfiguration();
    }

    protected @NotNull Configuration taskProcessorConfiguration() {
        return new DefaultTaskProcessorConfiguration();
    }

}
