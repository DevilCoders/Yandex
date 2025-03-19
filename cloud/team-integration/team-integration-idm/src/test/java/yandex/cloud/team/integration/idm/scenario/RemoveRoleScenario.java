package yandex.cloud.team.integration.idm.scenario;

import java.util.Map;

import org.assertj.core.api.Assertions;
import org.junit.Test;
import yandex.cloud.scenario.DependsOn;
import yandex.cloud.team.integration.idm.core.IdmServiceScenarioBase;
import yandex.cloud.team.integration.idm.model.response.GenericResponse;
import yandex.cloud.team.integration.idm.model.response.RolesResponse;

@DependsOn(AddRoleScenario.class)
public class RemoveRoleScenario extends IdmServiceScenarioBase {

    private static final String PATH = "/idm/remove-role";

    @Override
    public void main() {
        Assertions.assertThat(getAccessBindingsRequests()).isEmpty();
        var response = form(GenericResponse.class, PATH, getFields());
        Assertions.assertThat(response).isEqualTo(GenericResponse.OK);
        Assertions.assertThat(getAccessBindingsRequests()).hasSize(1);
        // todo assert that the request is to remove the access binding
    }

    @Test
    public void notAllowedMethodTest() {
        assertBadMethod(() -> get(RolesResponse.class, PATH));
    }

    @Test
    public void testNoRole() {
        var fields = getFields();
        fields.remove("role");
        assertBadRequest(() -> request(fields));
    }

    @Test
    public void testEmptyRole() {
        var fields = getFields();
        fields.put("role", "");
        assertBadRequest(() -> request(fields));
    }

    @Test
    public void testInvalidRole() {
        var fields = getFields();
        fields.put("role", "DEFINITELY_NOT_A_JSON_VALUE");
        assertBadRequest(() -> request(fields));
    }

    @Test
    public void testNullRoleCloudField() {
        var fields = getFields();
        fields.put("role", "{\"cloud\": null}");
        assertBadRequest(() -> request(fields));
    }

    @Test
    public void testEmptyRoleCloudField() {
        var fields = getFields();
        fields.put("role", "{\"cloud\": \"\"}");
        assertBadRequest(() -> request(fields));
    }

    @Test
    public void testUnknownRole() {
        var fields = getFields();
        fields.put("path", "/DEFINITELY/NOT/VALID/PATH");
        assertBadRequest(() -> request(fields));
    }

    @Test
    public void testNoFields() {
        var fields = getFields();
        fields.remove("fields");
        assertBadRequest(() -> request(fields));
    }

    @Test
    public void testEmptyFields() {
        var fields = getFields();
        fields.put("fields", "");
        assertBadRequest(() -> request(fields));
    }

    @Test
    public void testInvalidFields() {
        var fields = getFields();
        fields.put("fields", "DEFINITELY_NOT_A_JSON_VALUE");
        assertBadRequest(() -> request(fields));
    }

    @Test
    public void testNullServiceSpecField() {
        var fields = getFields();
        fields.put("fields", "{\"service_spec\": null}");
        assertBadRequest(() -> request(fields));
    }

    @Test
    public void testEmptyServiceSpecField() {
        var fields = getFields();
        fields.put("fields", "{\"service_spec\": \"\"}");
        assertBadRequest(() -> request(fields));
    }

    @Test
    public void testNotExistentServiceSpecField() {
        var fields = getFields();
        fields.put("fields", "{\"service_spec\": \"bar-1-1\"}");
        assertPreconditionFailed(() -> request(fields));
    }

    private GenericResponse request(Map<String, Object> fields) {
        return form(GenericResponse.class, PATH, fields);
    }

}
