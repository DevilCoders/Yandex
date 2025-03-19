package yandex.cloud.team.integration.abc;

import java.io.IOException;
import java.io.UncheckedIOException;

import javax.inject.Inject;

import io.grpc.Server;
import io.prometheus.client.CollectorRegistry;
import lombok.Value;
import yandex.cloud.common.httpserver.HttpServer;
import yandex.cloud.di.StaticDI;
import yandex.cloud.iam.repository.Bootstrap;
import yandex.cloud.repository.db.Repository;
import yandex.cloud.scenario.ScenarioContext;
import yandex.cloud.scenario.ScenarioSuite;
import yandex.cloud.scenario.contract.AbstractContractContext;
import yandex.cloud.task.TaskProcessorLifecycle;
import yandex.cloud.ti.abc.repo.AbcIntegrationRepository;
import yandex.cloud.ti.yt.abc.client.MockTeamAbcClient;
import yandex.cloud.ti.yt.abcd.client.MockTeamAbcdClient;

public class AbcIntegrationScenarioSuite extends ScenarioSuite {

    @Override
    public ScenarioContext<Snapshot> createContext() {
        CollectorRegistry.defaultRegistry.clear();
        StaticDI
                .inject(new TestAbcIntegrationConfiguration())
                .to(
                        "yandex.cloud.team.integration.abc", // this module
                        "yandex.cloud.iam.service",  // ServiceCallHandler
                        "yandex.cloud.repository",   // IdGenerator
                        "yandex.cloud.iam.grpc.remoteoperation", // RemoteOperationProcess
                        "yandex.cloud.ti"
                );
        return new Context();
    }

    public static class Context extends AbstractContractContext<Snapshot> {

        @Inject
        private static Bootstrap bootstrap;

        @Inject
        private static HttpServer httpServer;

        @Inject
        private static Server grpcServer;

        @Inject
        private static Repository repository;

        @Inject
        private static TaskProcessorLifecycle taskProcessorLifecycle;

        @Inject
        private static MockTeamAbcClient mockTeamAbcClient;
        @Inject
        private static MockTeamAbcdClient mockTeamAbcdClient;
        @Inject
        private static AbcIntegrationRepository abcIntegrationRepository;

        public MockTeamAbcClient getMockTeamAbcClient() {
            return mockTeamAbcClient;
        }

        public MockTeamAbcdClient getMockTeamAbcdClient() {
            return mockTeamAbcdClient;
        }

        public AbcIntegrationRepository getAbcIntegrationRepository() {
            return abcIntegrationRepository;
        }

        @Override
        public Snapshot snapshot() {
            return Snapshot.of(
                    repository.makeSnapshot()
            );
        }

        @Override
        public void load(Snapshot snapshot) {
            repository.loadSnapshot(snapshot.getRepositorySnapshotId());
        }

        @Override
        public void postConstruct() {
            bootstrap.run();
            httpServer.start();
            try {
                grpcServer.start();
            } catch (IOException e) {
                throw new UncheckedIOException(e);
            }
            taskProcessorLifecycle.start();
        }

        @Override
        public void preDestroy() {
            taskProcessorLifecycle.shutdown();
            grpcServer.shutdown();
            httpServer.stop();
        }

    }

    @Value(staticConstructor = "of")
    public static class Snapshot {

        String repositorySnapshotId;

    }

}
