package ru.yandex.ci.core.launch;

import com.fasterxml.jackson.core.type.TypeReference;
import com.google.common.base.Preconditions;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import org.assertj.core.api.InstanceOfAssertFactories;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.api.proto.Common;
import ru.yandex.ci.core.config.a.model.FlowVarsUi;
import ru.yandex.ci.core.tasklet.SchemaService;
import ru.yandex.ci.test.TestUtils;

import static org.assertj.core.api.Assertions.assertThat;
import static org.assertj.core.api.Assertions.assertThatThrownBy;

class FlowVarsServiceTest {
    private static final JsonObject EMPTY_JSON = new JsonObject();

    private FlowVarsService service;

    @BeforeEach
    public void setUp() {
        service = new FlowVarsService(new SchemaService());
    }

    @Test
    void invalidSchema() {
        assertThatThrownBy(() -> service.validate(EMPTY_JSON, flowVarsUi("""
                schema:
                    type: not-exists-type
                """)))
                .hasMessage("""
                        invalid json schema for flow vars: {"type":"not-exists-type"}""");
    }

    @Test
    void validSchemaEmptyJson() {
        var result = service.validate(EMPTY_JSON, flowVarsUi("""
                schema:
                    type: object
                """));

        assertThat(result.isValid()).isTrue();
    }

    @Test
    void invalidJson() {
        var result = service.validate(flowVars("""
                        {
                            "name": "Chuck"
                        }
                        """),
                flowVarsUi("""
                        schema:
                            type: object
                            properties:
                                name:
                                    type: string
                                    enum: ["Bob", "Alice"]
                        """));

        assertThat(result.getErrors())
                .singleElement(InstanceOfAssertFactories.STRING)
                .contains("""
                        instance value (\\"Chuck\\") not found in enum (possible values: [\\"Bob\\",\\"Alice\\"])""");
    }

    @Test
    void validJson() {
        var result = service.validate(flowVars("""
                        {
                            "name": "Alice",
                            "address": {
                                "city": "Moscow"
                            }
                        }
                        """),
                flowVarsUi("""
                        schema:
                            type: object
                            additionalProperties: false
                            properties:
                                name:
                                    type: string
                                    enum: ["Bob", "Alice"]
                                address:
                                    type: object
                                    additionalProperties: false
                                    properties:
                                        city:
                                            type: string
                        """));

        assertThat(result.isValid()).isTrue();
    }

    @Test
    void emptyFlowVarsWithRequiredAndDefaults() {
        var flowVarsUi = flowVarsUi("""
                schema:
                    type: object
                    additionalProperties: false
                    required: [ "version" ]
                    properties:
                        version:
                            title: Версия(Х.Х.Х)
                            type: string
                            default: "1.0.0"
                        versionToken:
                            title: Version token
                            type: string
                """);

        var declaredFlowVars = flowVars("""
                {"declared-in-yaml": 42}
                """);

        assertThat(service.prepareFlowVarsFromUi(null, declaredFlowVars, flowVarsUi))
                .isEqualTo(flowVars("""
                        {
                          "declared-in-yaml": 42,
                          "version": "1.0.0"
                        }
                        """));
    }

    @Test
    void makeDefaults() {
        var flowVarsUi = flowVarsUi("""
                schema:
                    type: object
                    additionalProperties: false
                    properties:
                        iteration:
                            type: integer
                        name:
                            type: string
                            enum: ["Bob", "Alice"]
                            default: Alice
                        address:
                            type: object
                            additionalProperties: false
                            properties:
                                city:
                                    type: string
                            default:
                                city: Moscow
                """);

        assertThat(service.makeDefaults(flowVarsUi))
                .isEqualTo(JsonParser.parseString("""
                        {
                          "name": "Alice",
                          "address": {
                            "city": "Moscow"
                          }
                        }
                        """));
    }

    private JsonObject flowVars(String json) {
        return Preconditions.checkNotNull(service.parse(Common.FlowVars.newBuilder().setJson(json).build()));
    }

    private FlowVarsUi flowVarsUi(String schemaInYaml) {
        var typeRef = new TypeReference<FlowVarsUi>() {
        };
        return TestUtils.parseYamlPartial(schemaInYaml, typeRef);
    }
}
