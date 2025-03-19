package yandex.cloud.team.integration.idm.http.servlet;

import org.assertj.core.api.Assertions;
import org.junit.Test;
import yandex.cloud.team.integration.idm.exception.InvalidServiceSpecException;
import yandex.cloud.team.integration.idm.exception.MissingFieldException;
import yandex.cloud.team.integration.idm.model.request.FieldsParameter;

public class FieldsParameterTest {

    @Test
    public void testParse() {
        var value = FieldsParameter.valueOf("{\"service_spec\": \"abc-989-42868\"}");
        Assertions.assertThat(value).isEqualTo(new FieldsParameter("abc", 989));
    }

    @Test
    public void testParseAbcSlugWithDashes() {
        var value = FieldsParameter.valueOf("{\"service_spec\": \"-some--abc-service-with-dashes---989-42868\"}");
        Assertions.assertThat(value).isEqualTo(new FieldsParameter("-some--abc-service-with-dashes--", 989));
    }

    @Test
    public void testEmpty() {
        Assertions.assertThatThrownBy(() -> FieldsParameter.valueOf("{}"))
            .isInstanceOf(MissingFieldException.class)
            .hasMessage("The field 'service_spec' of the parameter 'fields' is missing");
    }


    @Test
    public void testNullServiceSpec() {
        Assertions.assertThatThrownBy(() -> FieldsParameter.valueOf("{\"service_spec\": null}"))
            .isInstanceOf(MissingFieldException.class)
            .hasMessage("The field 'service_spec' of the parameter 'fields' is missing");
    }

    @Test
    public void testEmptyServiceSpec() {
        Assertions.assertThatThrownBy(() -> FieldsParameter.valueOf("{\"service_spec\": \"\"}"))
            .isInstanceOf(MissingFieldException.class)
            .hasMessage("The field 'service_spec' of the parameter 'fields' is missing");
    }

    @Test
    public void testMissingAbcId() {
        Assertions.assertThatThrownBy(() -> FieldsParameter.valueOf("{\"service_spec\": \"only.slug\"}"))
            .isInstanceOf(InvalidServiceSpecException.class)
            .hasMessage("The field 'service_spec' of the parameter 'fields' should be in format 'slug-id-group', got 'only.slug'");
    }

    @Test
    public void testEmptyAbcId() {
        Assertions.assertThatThrownBy(() -> FieldsParameter.valueOf("{\"service_spec\": \"slug--123\"}"))
            .isInstanceOf(InvalidServiceSpecException.class)
            .hasMessage("The field 'service_spec' of the parameter 'fields' should be in format 'slug-id-group', got 'slug--123'");
    }

    @Test
    public void testMissingGroupId() {
        Assertions.assertThatThrownBy(() -> FieldsParameter.valueOf("{\"service_spec\": \"slug-123\"}"))
            .isInstanceOf(InvalidServiceSpecException.class)
            .hasMessage("The field 'service_spec' of the parameter 'fields' should be in format 'slug-id-group', got 'slug-123'");
    }

    @Test
    public void testEmptyAbcGroupId() {
        // We are not using group id, so this field is optional.
        var value = FieldsParameter.valueOf("{\"service_spec\": \"abc-989-\"}");
        Assertions.assertThat(value).isEqualTo(new FieldsParameter("abc", 989));
    }

    @Test
    public void testMalformedAbcId() {
        Assertions.assertThatThrownBy(() -> FieldsParameter.valueOf("{\"service_spec\": \"slug-not_integer\"}"))
            .isInstanceOf(InvalidServiceSpecException.class)
            .hasMessage("The field 'service_spec' of the parameter 'fields' should be in format 'slug-id-group', got 'slug-not_integer'");
    }

}
