package ru.yandex.ci.client.sandbox.api;

import java.util.List;
import java.util.Map;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.MethodSource;

import static org.assertj.core.api.Assertions.assertThat;

class SandboxCustomFieldTest {

    private ObjectMapper objectMapper;

    @BeforeEach
    void setUp() {
        objectMapper = new ObjectMapper();
    }

    @ParameterizedTest
    @MethodSource
    void serializeDeserialize(SandboxCustomField original) throws JsonProcessingException {
        var json = objectMapper.writeValueAsString(original);
        var parsed = objectMapper.readValue(json, SandboxCustomField.class);
        assertThat(parsed).isEqualTo(original);
    }

    static List<SandboxCustomField> serializeDeserialize() {
        String key = "key";
        return List.of(
                new SandboxCustomField(key, "string value"),
                new SandboxCustomField(key, 42),
                new SandboxCustomField(key, Long.MAX_VALUE),
                new SandboxCustomField(key, List.of("one", "two", "tree")),
                new SandboxCustomField(key, Map.of("key", List.of(Map.of("nested", "one"), "two", 42)))
        );

    }
}
