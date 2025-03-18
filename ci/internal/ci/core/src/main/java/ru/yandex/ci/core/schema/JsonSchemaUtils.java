package ru.yandex.ci.core.schema;

import com.fasterxml.jackson.databind.JsonNode;

public class JsonSchemaUtils {
    private JsonSchemaUtils() {
    }

    public static void walkProperties(JsonNode schema, PropertiesVisitor visitor) {
        visitor.visit(schema);
        var properties = schema.path("properties");
        if (properties.isObject()) {
            for (var property : properties) {
                walkProperties(property, visitor);
            }
        }

        var patternProperties = schema.path("patternProperties");
        if (patternProperties.isObject()) {
            for (var property : patternProperties) {
                walkProperties(property, visitor);
            }
        }

        var items = schema.path("items");
        if (items.isObject()) {
            walkProperties(items, visitor);
        }
    }

    public interface PropertiesVisitor {
        void visit(JsonNode property);
    }
}
