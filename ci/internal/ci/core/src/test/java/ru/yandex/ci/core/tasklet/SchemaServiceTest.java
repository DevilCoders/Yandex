package ru.yandex.ci.core.tasklet;

import com.google.gson.JsonObject;
import com.google.protobuf.InvalidProtocolBufferException;
import io.github.benas.randombeans.EnhancedRandomBuilder;
import io.github.benas.randombeans.api.EnhancedRandom;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.ValueSource;

import ru.yandex.ci.test.TestUtils;
import ru.yandex.ci.test.random.TestRandomUtils;

import static org.assertj.core.api.Assertions.assertThat;
import static org.assertj.core.api.Assertions.assertThatThrownBy;

class SchemaServiceTest {
    private static final long SEED = 705359839L;

    private SchemaService service;
    private final EnhancedRandom random = new EnhancedRandomBuilder()
            .seed(SEED)
            .build();

    @BeforeEach
    public void setUp() {
        service = new SchemaService();
    }

    @Test
    public void validSchema() {
        service.validate("Tasklet.Test.Input", "Tasklet.Test.Output", validBinaryResource());
    }

    @Test
    public void cannotParse() {
        assertThatThrownBy(() -> {
            service.validate("Tasklet.Test.Input", "Tasklet.Test.Output", TestRandomUtils.bytes(random));
        }).isInstanceOf(SchemaValidationException.class)
                .hasMessage("Unable to parse descriptors from bytes")
                .hasCauseInstanceOf(InvalidProtocolBufferException.class);
    }

    @Test
    public void cannotFindInputMessage() {
        assertThatThrownBy(() -> {
            service.validate("Tasklet.Test.NotExists", "Tasklet.Test.Output", validBinaryResource());
        }).isInstanceOf(SchemaValidationException.class)
                .hasMessageContaining("cannot find input message type Tasklet.Test.NotExists in descriptors");
    }

    @Test
    public void cannotFindOutputMessage() {
        assertThatThrownBy(() -> {
            service.validate("Tasklet.Test.Input", "Tasklet.Test.NotExists", validBinaryResource());
        }).isInstanceOf(SchemaValidationException.class)
                .hasMessageContaining("cannot find output message type Tasklet.Test.NotExists in descriptors");
    }

    @Test
    void overrideNullBase() {
        JsonObject override = new JsonObject();
        assertThat(service.override(null, override)).isSameAs(override);
    }

    @Test
    void overrideNullOverrides() {
        JsonObject base = new JsonObject();
        assertThat(service.override(base, null)).isSameAs(base);
    }

    @Test
    void overrideNullBoth() {
        assertThat(service.override(null, null)).isNull();
    }

    @Test
    void overrideObjectsNullBase() {
        JsonObject override = new JsonObject();
        assertThat(service.override(null, override)).isSameAs(override);
    }

    @Test
    void overrideObjectsNullOverrides() {
        JsonObject base = new JsonObject();
        assertThat(service.override(base, null)).isSameAs(base);
    }

    @Test
    void overrideObjectsNullBoth() {
        assertThat(service.override(null, null)).isNull();
    }

    @ParameterizedTest
    @ValueSource(strings = {
            "remove-property",
            "change-type",
            "primitive",
            "add-property",
            "replace-list"
    })
    void overrideTest(String fileName) {
        testOverride("json-override", fileName, false);
    }

    @ParameterizedTest
    @ValueSource(strings = {
            "camel-to-snake",
            "snake-to-camel",
            "upper-case",
            "ignore-case",
            "override-map"
    })
    void overrideWithSnakeCaseTest(String fileName) {
        testOverride("json-override-snakecase", fileName, true);
    }

    private void testOverride(String prefix, String fileName, boolean matchSnakeCase) {
        var base = TestUtils.parseGson(prefix + "/base.json").getAsJsonObject();
        var override = TestUtils.parseGson(prefix + "/" + fileName + ".json").getAsJsonObject();
        var expected = TestUtils.parseGson(prefix + "/" + fileName + ".result.json").getAsJsonObject();

        var baseCopy = base.deepCopy();
        var overrideCopy = override.deepCopy();

        var actual = service.override(base, override, matchSnakeCase);
        assertThat(actual)
                .isEqualTo(expected);

        assertThat(baseCopy).isEqualTo(base);
        assertThat(overrideCopy).isEqualTo(override);
    }

    private byte[] validBinaryResource() {
        return TestUtils.binaryResource("tasklet-schema/descriptors.bin");
    }
}
