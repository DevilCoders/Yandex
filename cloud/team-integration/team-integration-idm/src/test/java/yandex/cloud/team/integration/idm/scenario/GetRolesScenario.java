package yandex.cloud.team.integration.idm.scenario;

import org.assertj.core.api.Assertions;
import org.junit.Test;
import yandex.cloud.scenario.DependsOn;
import yandex.cloud.team.integration.idm.IdmServiceScenarioSuite;
import yandex.cloud.team.integration.idm.core.IdmServiceScenarioBase;
import yandex.cloud.team.integration.idm.model.response.GenericResponse;

@DependsOn(IdmServiceScenarioSuite.class)
public class GetRolesScenario extends IdmServiceScenarioBase {

    private static final String PATH = "/idm/get-roles";

    @Override
    public void main() {
        var response =  get(GenericResponse.class, PATH);
        Assertions.assertThat(response).isNotNull();
    }

    @Test
    public void notAllowedMethodTest() {
        assertBadMethod(() -> form(GenericResponse.class, PATH, getFields()));
    }

}
