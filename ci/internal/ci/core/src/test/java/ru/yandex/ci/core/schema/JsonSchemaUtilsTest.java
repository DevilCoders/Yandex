package ru.yandex.ci.core.schema;

import java.util.ArrayList;
import java.util.List;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.JsonNode;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.core.config.a.AYamlParser;

import static org.assertj.core.api.Assertions.assertThat;

class JsonSchemaUtilsTest {
    private Visitor visitor;

    @BeforeEach
    public void setUp() {
        visitor = new Visitor();
    }

    @Test
    void walk() {
        JsonSchemaUtils.walkProperties(yaml("""
                type: object
                id: root
                properties:
                    name:
                        id: name
                        type: string
                    children:
                        id: children
                        type: array
                        items:
                            id: children-property
                            type: number
                    loan:
                        id: loan
                        type: object
                        patternProperties:
                            "a[0-9]+":
                                id: a
                                type: string
                            "b.*":
                                id: b
                                type: number
                """), visitor);

        assertThat(visitor.getVisited()).containsExactly(
                "root",
                "name",
                "children",
                "children-property",
                "loan",
                "a",
                "b"
        );
    }

    private static class Visitor implements JsonSchemaUtils.PropertiesVisitor {
        private final List<String> visited = new ArrayList<>();

        @Override
        public void visit(JsonNode property) {
            visited.add(property.get("id").asText());
        }

        public List<String> getVisited() {
            return visited;
        }
    }

    private static JsonNode yaml(String yaml) {
        try {
            return AYamlParser.getMapper().readTree(yaml);
        } catch (JsonProcessingException e) {
            throw new RuntimeException(e);
        }
    }
}
