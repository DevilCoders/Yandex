package yandex.cloud.ti.abcd.adapter;

import java.io.IOException;
import java.io.UncheckedIOException;

import javax.inject.Inject;

import io.grpc.Server;
import io.grpc.ServerBuilder;
import io.grpc.inprocess.InProcessServerBuilder;
import io.prometheus.client.CollectorRegistry;
import lombok.Value;
import org.jetbrains.annotations.NotNull;
import yandex.cloud.di.Configuration;
import yandex.cloud.di.StaticDI;
import yandex.cloud.grpc.FakeServerInterceptors;
import yandex.cloud.scenario.ScenarioContext;
import yandex.cloud.scenario.ScenarioSuite;
import yandex.cloud.scenario.contract.AbstractContractContext;
import yandex.cloud.ti.yt.abcd.client.MockTeamAbcdClient;

public class AbcdAdapterScenarioSuite extends ScenarioSuite {

    @Override
    public ScenarioContext<Snapshot> createContext() {
        CollectorRegistry.defaultRegistry.clear();
        StaticDI
                .inject(createConfiguration())
                .to(
                        AbcdAdapterScenarioSuite.class.getPackageName(),
                        "yandex.cloud.ti",
                        "yandex.cloud.iam.service",  // ServiceCallHandler
                        "yandex.cloud.repository",   // IdGenerator
                        "yandex.cloud.iam.grpc.remoteoperation" // RemoteOperationProcess
                );
        return new Context();
    }

    public Configuration createConfiguration() {
        return new Configuration() {
            @Override
            protected void configure() {
                merge(new TestAbcdAdapterConfiguration());
            }
        };
    }

    public static class Context extends AbstractContractContext<Snapshot> {

        @Inject
        private static Server grpcServer;

        @Inject
        private static MockTeamAbcdClient mockTeamAbcdClient;

        public MockTeamAbcdClient getMockTeamAbcdClient() {
            return mockTeamAbcdClient;
        }

        @Override
        public Snapshot snapshot() {
            return Snapshot.of(
            );
        }

        @Override
        public void load(Snapshot snapshot) {
        }

        @Override
        public void postConstruct() {
            try {
                grpcServer.start();
            } catch (IOException e) {
                throw new UncheckedIOException(e);
            }
        }

        @Override
        public void preDestroy() {
            grpcServer.shutdown();
        }

        private static @NotNull ServerBuilder<?> server(
                @NotNull String serviceName,
                @NotNull String channelName
        ) {
            return FakeServerInterceptors.intercept(serviceName, InProcessServerBuilder.forName(channelName),
                    FakeServerInterceptors.InterceptorConfig.VERBOSE);
        }

    }

    @Value(staticConstructor = "of")
    public static class Snapshot {
    }

}
