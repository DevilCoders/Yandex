package yandex.cloud.team.integration.idm.core;

import javax.inject.Inject;

import lombok.AllArgsConstructor;
import lombok.Value;
import yandex.cloud.common.httpserver.HttpServer;
import yandex.cloud.fake.iam.FakeIamServer;
import yandex.cloud.metrics.MetricReporter;
import yandex.cloud.scenario.ScenarioContext;

public interface IdmServiceTestContext extends ScenarioContext<IdmServiceTestContext.Snapshot> {

    FakeIamServer getIamServer();

    TestInit getInit();

    @Override
    default void postConstruct() {
        getInit().start();
    }

    @Override
    default void preDestroy() {
        getInit().stop();
    }

    @Override
    default void beforeTest() {
        resetBackdoors();
    }

    @Override
    default Snapshot snapshot() {
        return new Snapshot(
                getIamServer().makeSnapshot()
        );
    }

    @Override
    default void load(Snapshot snapshot) {
        getIamServer().loadSnapshot(snapshot.iamServerNewState);
    }

    default void resetBackdoors() {
        getIamServer().resetBackdoor();
    }

    @Value
    class Snapshot {

        FakeIamServer.State iamServerNewState;

    }

    @AllArgsConstructor
    class TestInit {

        @Inject
        private static HttpServer httpServer;

        @Inject
        private static MetricReporter metricReporter;

        final FakeIamServer fakeIamServer;


        public void start() {
            fakeIamServer.start();
            metricReporter.start();
            httpServer.start();
        }

        public void stop() {
            httpServer.stop();
            metricReporter.stop();
            fakeIamServer.stop();
        }

    }

}
