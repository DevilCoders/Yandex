package yandex.cloud.team.integration.idm.scenario;

import org.assertj.core.api.Assertions;
import org.junit.Test;
import yandex.cloud.scenario.DependsOn;
import yandex.cloud.team.integration.idm.IdmServiceScenarioSuite;
import yandex.cloud.team.integration.idm.core.IdmServiceScenarioBase;
import yandex.cloud.team.integration.idm.model.response.RolesResponse;

@DependsOn(IdmServiceScenarioSuite.class)
public class InfoScenario extends IdmServiceScenarioBase {

    private static final String PATH = "/idm/info";

    @Override
    public void main() {
        var response = get(RolesResponse.class, PATH);
        Assertions.assertThat(response).isNotNull();
    }

    @Test
    public void notAllowedMethodTest() {
        assertBadMethod(() -> form(RolesResponse.class, PATH, getFields()));
    }

}
