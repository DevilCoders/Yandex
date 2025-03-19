package yandex.cloud.team.integration.idm.http.servlet;

import org.assertj.core.api.Assertions;
import org.junit.Test;
import yandex.cloud.team.integration.idm.model.request.RoleParameter;

public class RoleParameterTest {

    @Test
    public void testParse() {
        var value = RoleParameter.valueOf("{\"cloud\": \"bar\", \"storage\": \"foo\"}");
        Assertions.assertThat(value).isEqualTo(new RoleParameter("bar", "foo"));
    }

    @Test
    public void testEmpty() {
        var value = RoleParameter.valueOf("{}");
        Assertions.assertThat(value).isEqualTo(new RoleParameter(null, null));
    }

    @Test
    public void testNullS3Bucket() {
        var value = RoleParameter.valueOf("{\"role\": null}");
        Assertions.assertThat(value).isEqualTo(new RoleParameter(null, null));
    }

}
