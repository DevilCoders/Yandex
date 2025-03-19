package yandex.cloud.team.integration.idm;

import io.prometheus.client.CollectorRegistry;
import yandex.cloud.di.Configuration;
import yandex.cloud.di.StaticDI;
import yandex.cloud.scenario.ScenarioSuite;
import yandex.cloud.team.integration.idm.config.TestConfiguration;
import yandex.cloud.team.integration.idm.core.IdmServiceTestContext;
import yandex.cloud.team.integration.idm.core.IdmServiceTestContextImpl;

public class IdmServiceScenarioSuite extends ScenarioSuite {

    @Override
    public IdmServiceTestContext createContext() {
        CollectorRegistry.defaultRegistry.clear();
        StaticDI.inject(createConfiguration()).to(
            "yandex.cloud.team.integration.idm" // this module
        );
        return new IdmServiceTestContextImpl();
    }

    public Configuration createConfiguration() {
        return new Configuration() {
            @Override
            protected void configure() {
                merge(new TestConfiguration());
            }
        };
    }

}
