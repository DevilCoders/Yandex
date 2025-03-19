package yandex.cloud.ti.abcd.adapter;

import java.time.Clock;

import io.grpc.inprocess.InProcessChannelBuilder;
import org.jetbrains.annotations.NotNull;
import yandex.cloud.auth.api.CloudAuthClient;
import yandex.cloud.auth.api.FakeCloudAuthClient;
import yandex.cloud.auth.api.Subject;
import yandex.cloud.auth.api.credentials.AbstractCredentials;
import yandex.cloud.di.Configuration;
import yandex.cloud.grpc.ExceptionMapper;
import yandex.cloud.grpc.GrpcHeaders;
import yandex.cloud.grpc.HeaderAttachingClientInterceptor;
import yandex.cloud.iam.grpc.GrpcCallHandler;
import yandex.cloud.iam.grpc.IamExceptionMapper;
import yandex.cloud.lang.test.time.TestClock;
import yandex.cloud.team.integration.abc.FakeResolveServiceFacade;
import yandex.cloud.team.integration.abc.ResolveServiceFacade;
import yandex.cloud.ti.abcd.provider.AbcdProvider;
import yandex.cloud.ti.abcd.provider.TestAbcdProviders;
import yandex.cloud.ti.grpc.server.GrpcServerConfiguration;
import yandex.cloud.ti.grpc.server.TestGrpcServerConfiguration;
import yandex.cloud.ti.tvm.client.TestTvmClientConfiguration;
import yandex.cloud.ti.yt.abcd.client.MockTeamAbcdClientConfiguration;

import ru.yandex.intranet.d.backend.service.provider_proto.AccountsServiceGrpc;

public class TestAbcdAdapterConfiguration extends Configuration {

    public static final String NAME = "AbcdAdapterTest";

    @Override
    protected void configure() {
        put(ResolveServiceFacade.class, this::resolveServiceFacade);

        merge(new AbcdAdapterConfiguration(NAME) {
            // todo refactor to extend AbcdAdapterConfiguration instead of merging it
            //  maybe extract TestFixturesAbcdAdapterConfiguration first as this one creates a lot of use stuff
            //  or extract "other stuff" into reusable test config
            @Override
            protected @NotNull AbcdProvider[] createAbcdProvidersArray() {
                return new AbcdProvider[]{
                        TestAbcdProviders.testAbcdProvider()
                };
            }
        });

        merge(new MockTeamAbcdClientConfiguration(NAME));
        put(Clock.class, TestClock::new);
        put(CloudAuthClient.class, this::createCloudAuthClient);
        put(ExceptionMapper.class, IamExceptionMapper::new);
        put(GrpcCallHandler.class, () -> new GrpcCallHandler(get(ExceptionMapper.class)));
        merge(new TestTvmClientConfiguration());
        merge(grpcServerConfiguration());
    }

    private ResolveServiceFacade resolveServiceFacade() {
        return new FakeResolveServiceFacade();
    }

    private CloudAuthClient createCloudAuthClient() {
        return new FakeCloudAuthClient() {

            @Override
            public Subject authenticate(AbstractCredentials credentials) {
                return Subject.Anonymous::id;
            }

        };
    }

    private @NotNull GrpcServerConfiguration grpcServerConfiguration() {
        return new TestGrpcServerConfiguration(
                NAME,
                AbcdAdapterConfiguration.getBindableServiceClasses()
        );
    }


    static @NotNull AccountsServiceGrpc.AccountsServiceBlockingStub createTestAccountsServiceClient() {
        return AccountsServiceGrpc
                .newBlockingStub(InProcessChannelBuilder
                        .forName(NAME)
                        .directExecutor()
                        .build()
                )
                .withInterceptors(
                        new HeaderAttachingClientInterceptor(GrpcHeaders.X_YA_SERVICE_TICKET, "TVM:0:0")
                );
    }

}
