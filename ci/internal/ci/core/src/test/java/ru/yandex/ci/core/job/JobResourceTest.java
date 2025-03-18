package ru.yandex.ci.core.job;

import com.google.gson.JsonObject;
import com.google.gson.JsonPrimitive;
import org.assertj.core.api.Assertions;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.core.test.schema.SimpleEnum;

class JobResourceTest {

    @Test
    void protoDefaultValue() {

        JsonObject expected = new JsonObject();
        expected.add("value", new JsonPrimitive("DEFAULT"));

        SimpleEnum simpleEnum = SimpleEnum.newBuilder().setValue(SimpleEnum.Enum.DEFAULT).build();

        Assertions.assertThat(JobResource.regular(simpleEnum).getData())
                .isEqualTo(expected);

    }
}
