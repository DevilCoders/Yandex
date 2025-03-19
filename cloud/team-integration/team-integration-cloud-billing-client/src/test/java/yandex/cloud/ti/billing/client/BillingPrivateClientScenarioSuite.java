package yandex.cloud.ti.billing.client;

import javax.inject.Inject;

import yandex.cloud.common.httpserver.HttpServer;
import yandex.cloud.di.Configuration;
import yandex.cloud.di.StaticDI;
import yandex.cloud.scenario.ScenarioContext;
import yandex.cloud.scenario.ScenarioSuite;
import yandex.cloud.scenario.contract.AbstractContractContext;

public class BillingPrivateClientScenarioSuite extends ScenarioSuite {

    @Override
    public ScenarioContext<Object> createContext() {
        StaticDI
                .inject(createConfiguration())
                .to(
                        BillingPrivateClient.class.getPackageName()
                );
        return new Context();
    }

    public Configuration createConfiguration() {
        return new Configuration() {
            @Override
            protected void configure() {
                merge(new TestBillingPrivateClientConfiguration());
            }
        };
    }

    public static class Context extends AbstractContractContext<Object> {

        @Inject
        private static HttpServer httpServer;

        @Override
        public Object snapshot() {
            return new Object();
        }

        @Override
        public void load(Object snapshot) {
        }

        @Override
        public void postConstruct() {
            httpServer.start();
        }

        @Override
        public void preDestroy() {
            httpServer.stop();
        }

    }

}
