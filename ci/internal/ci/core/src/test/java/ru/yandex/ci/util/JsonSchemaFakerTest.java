package ru.yandex.ci.util;

import java.util.stream.Stream;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.JsonNode;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.Arguments;
import org.junit.jupiter.params.provider.MethodSource;

import ru.yandex.ci.core.config.a.AYamlParser;
import ru.yandex.ci.core.schema.FakeDataFactory;
import ru.yandex.ci.core.schema.JsonSchemaFaker;

import static org.assertj.core.api.Assertions.assertThat;

class JsonSchemaFakerTest {
    private JsonSchemaFaker faker;

    @BeforeEach
    public void setUp() {
        faker = new JsonSchemaFaker(new FakeDataFactory());
    }

    @ParameterizedTest
    @MethodSource
    void simple(String schema, String result) {
        var actual = faker.tryMakeValueBySchema(yaml(schema), false);
        var expected = yaml(result);
        assertThat(actual).isEqualTo(expected);
    }

    @Test
    void onlyRequired() {
        var schema = yaml("""
                type: object
                required: [ address ]
                properties:
                    name:
                        type: string
                    address:
                        type: object
                        required: [ city, zip ]
                        properties:
                            zip:
                                type: number
                            country:
                                type: string
                """);
        var actual = faker.tryMakeValueBySchema(schema, true);
        assertThat(actual).isEqualTo(yaml("""
                address:
                    zip: 3
                    city: Monica
                """));
    }

    @Test
    void withoutExplicitObjectType() {
        var schema = yaml("""
                properties:
                    name:
                        type: string
                    address:
                        properties:
                            zip:
                                type: number
                            country:
                                type: string
                """);
        var actual = faker.tryMakeValueBySchema(schema, false);
        assertThat(actual).isEqualTo(yaml("""
                name: Monica
                address:
                    zip: 3
                    country: Monica
                """));
    }

    @Test
    void withUniqueItemsInArray() {
        var schema = yaml("""
                required: [ name ]
                properties:
                    name:
                        type: array
                        uniqueItems: true
                        items:
                            type: string
                            enum:
                                - name_1
                """);
        var actual = faker.tryMakeValueBySchema(schema, false);
        assertThat(actual).isEqualTo(yaml("""
                name: ["name_1"]
                """));
    }

    static Stream<Arguments> simple() {
        return Stream.of(
                testCase("type: string", "Monica"),
                testCase("type: number", "3"),
                testCase("type: integer", "3"),
                testCase("type: boolean", "true"),
                testCase("type: \"null\"", "null"),
                testCase("type: array", "[\"Monica\", 3, true]"),
                testCase("""
                        type: string
                        enum: [ hello, bye ]
                        """, "hello"),
                testCase("""
                        type: number
                        enum: [ 2, 17 ]
                        """, "2"),
                testCase("""
                        type: number
                        maximum: 10
                        """, "8"),
                testCase("""
                        type: number
                        minimum: 10
                        """, "11"),
                testCase("""
                        type: number
                        minimum: 0
                        maximum: 100
                        """, "50"),
                testCase("""
                        type: array
                        minItems: 4
                        """, "[\"Monica\", 3, true, \"Monica\", 3]"),
                testCase("""
                        type: array
                        maxItems: 3
                        """, "[\"Monica\"]"),
                testCase("""
                        type: array
                        minItems: 2
                        maxItems: 3
                        """, "[\"Monica\", 3]"),
                testCase("""
                        type: array
                        minItems: 2
                        maxItems: 2
                        items:
                            type: number
                        """, "[3, 3]"),
                testCase("""
                        type: array
                        enum:
                            - one
                            - two
                            - three
                        """, "[one, two, three]"),
                testCase("""
                        type: array
                        minItems: 2
                        maxItems: 2
                        items:
                            type: string
                            enum: [ hello, bye ]
                        """, "[hello, hello]"),
                testCase("""
                        type: object
                        """, "{\"Monica\":\"Monica\"}"),
                testCase("""
                        type: object
                        required: [ path, title ]
                        """, "{\"path\":\"Monica\",\"title\":\"Monica\"}"),
                testCase("""
                        type: object
                        required: [ path ]
                        properties:
                            data:
                                type: number
                        """, "{\"path\":\"Monica\",\"data\":3}")
        );
    }

    private static Arguments testCase(String schema, String result) {
        return Arguments.of(schema, result);
    }

    private static JsonNode yaml(String yaml) {
        try {
            return AYamlParser.getMapper().readTree(yaml);
        } catch (JsonProcessingException e) {
            throw new RuntimeException(e);
        }
    }
}
