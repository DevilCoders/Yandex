package ru.yandex.ci.core.schema;

import java.util.List;
import java.util.Optional;
import java.util.Set;
import java.util.function.Supplier;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import javax.annotation.Nullable;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.ArrayNode;
import com.fasterxml.jackson.databind.node.BooleanNode;
import com.fasterxml.jackson.databind.node.IntNode;
import com.fasterxml.jackson.databind.node.MissingNode;
import com.fasterxml.jackson.databind.node.NullNode;
import com.fasterxml.jackson.databind.node.NumericNode;
import com.fasterxml.jackson.databind.node.ObjectNode;
import com.fasterxml.jackson.databind.node.TextNode;
import com.google.common.base.Preconditions;
import com.google.common.collect.Lists;
import lombok.RequiredArgsConstructor;
import lombok.Value;
import one.util.streamex.EntryStream;
import one.util.streamex.StreamEx;

import ru.yandex.ci.util.CiJson;

/**
 * По json-схеме делает json, который бы прошел валидацию.
 * Используется для генерации flow-vars из flow-vars-ui схемы и валидации подстановок jmespath против этих значений.
 * <p>
 * Не поддержана куча вещей:
 * <pre>
 * - onyOf
 * - patternProperties
 * - $ref
 * - форматы строк, в том числе regex
 * - range для number
 * - array uniq
 * - ...
 * </pre>
 */
public class JsonSchemaFaker {
    private static final int DEFAULT_RANGE_SIZE = 3;
    private static final Range SINGLE_ITEM_RANGE = Range.of(1, 1);
    private static final Range DEFAULT_RANGE = Range.of(2, 2 + DEFAULT_RANGE_SIZE);

    private final FakeDataFactory dataFactory;
    private final ObjectMapper mapper = CiJson.mapper();

    private final JsonNode simpleStringSchema;
    private final JsonNode simpleNumberSchema;
    private final JsonNode simpleBooleanSchema;

    public JsonSchemaFaker(FakeDataFactory dataFactory) {
        this.dataFactory = dataFactory;
        try {
            simpleStringSchema = mapper.readTree("""
                    {
                        "type": "string"
                    }
                    """);
            simpleNumberSchema = mapper.readTree("""
                    {
                        "type": "number"
                    }
                    """);
            simpleBooleanSchema = mapper.readTree("""
                    {
                        "type": "boolean"
                    }
                    """);
        } catch (JsonProcessingException e) {
            throw new RuntimeException(e);
        }
    }

    /**
     * Ад и содомия. Попытка собрать такой json, чтобы он соответствовал схеме.
     *
     * @param onlyRequired генерировать только поля, которые указаны в required.
     */
    public JsonNode tryMakeValueBySchema(JsonNode schema, boolean onlyRequired) {
        // http://json-schema.org/understanding-json-schema/reference/type.html
        return switch (schema.path("type").asText()) {
            case "string" -> makeString(schema);
            case "number", "integer" -> makeNumber(schema);
            case "boolean" -> makeBoolean();
            case "null" -> makeNull();
            case "array" -> makeArray(schema, onlyRequired);
            case "object" -> makeObject(schema, onlyRequired);
            default -> {
                if (!schema.path("properties").isMissingNode()) {
                    yield makeObject(schema, onlyRequired);
                }
                yield MissingNode.getInstance();
            }
        };

    }

    private ObjectNode makeObject(JsonNode schema, boolean onlyRequired) {
        var properties = EntryStream.of(schema.path("properties").fields()).toMap();
        var required = stringsSetFromArray(schema.get("required"));

        var result = mapper.createObjectNode();
        if (onlyRequired) {
            for (var fieldName : required) {
                var value = tryMakeValueBySchema(properties.getOrDefault(fieldName, simpleStringSchema), onlyRequired);
                result.set(fieldName, value);
            }
        } else {
            var fieldNames = StreamEx.of(properties.keySet())
                    .append(required)
                    .toSet();

            if (fieldNames.isEmpty()) {
                fieldNames = Stream.generate(dataFactory::nextString)
                        .limit(DEFAULT_RANGE_SIZE)
                        .collect(Collectors.toSet());
            }
            for (var fieldName : fieldNames) {
                var value = tryMakeValueBySchema(properties.getOrDefault(fieldName, simpleStringSchema), onlyRequired);
                result.set(fieldName, value);
            }
        }

        return result;
    }

    private Set<String> stringsSetFromArray(@Nullable JsonNode node) {
        if (node == null) {
            return Set.of();
        }

        Preconditions.checkArgument(node.isArray());
        return StreamEx.of(node.elements())
                .map(JsonNode::textValue)
                .toSet();
    }

    private ArrayNode makeArray(JsonNode schema, boolean onlyRequired) {
        var arrayNode = mapper.createArrayNode();
        Supplier<JsonNode> itemFactory;
        if (schema.get("enum") != null) {
            itemFactory = new CircularEnumFactory(Lists.newArrayList(schema.get("enum").elements()));
        } else if (schema.get("items") != null) {
            var itemsSchema = schema.get("items");
            itemFactory = new SingleTypeFactory(itemsSchema, onlyRequired);
        } else {
            itemFactory = new CircularTypeFactory(
                    List.of(simpleStringSchema, simpleNumberSchema, simpleBooleanSchema),
                    onlyRequired
            );
        }

        var uniqueItems = Optional.ofNullable(schema.get("uniqueItems"))
                .map(JsonNode::asBoolean)
                .orElse(false);

        var range = makeRange(schema.get("minItems"), schema.get("maxItems"), uniqueItems);
        for (int i = 0; i < pickValueFrom(range); i++) {
            arrayNode.add(itemFactory.get());
        }
        return arrayNode;
    }

    private static int pickValueFrom(Range range) {
        return (range.getMin() + range.getMax()) / 2;
    }

    private static Range makeRange(@Nullable JsonNode minValue, @Nullable JsonNode maxValue, boolean uniqueItems) {
        if (uniqueItems) {
            return SINGLE_ITEM_RANGE;
        }

        if (minValue == null && maxValue == null) {
            return DEFAULT_RANGE;
        }

        if (minValue != null && maxValue != null) {
            return Range.of(minValue.asInt(), maxValue.asInt());
        }

        if (minValue != null) {
            return Range.of(minValue.asInt(), minValue.asInt() + DEFAULT_RANGE_SIZE);
        }

        var min = Math.max(0, maxValue.asInt() - DEFAULT_RANGE_SIZE);
        return Range.of(min, maxValue.asInt());
    }

    private NullNode makeNull() {
        return NullNode.getInstance();
    }

    private BooleanNode makeBoolean() {
        return BooleanNode.valueOf(dataFactory.nextBoolean());
    }

    private NumericNode makeNumber(JsonNode schema) {
        if (schema.has("enum")) {
            return IntNode.valueOf(schema.get("enum").get(0).asInt());
        }
        var range = makeRange(schema.get("minimum"), schema.get("maximum"), false);
        return IntNode.valueOf(dataFactory.nextInteger(range.getMin(), range.getMax()));
    }

    private TextNode makeString(JsonNode schema) {
        if (schema.has("enum")) {
            return TextNode.valueOf(schema.get("enum").get(0).asText());
        }
        return TextNode.valueOf(dataFactory.nextString());
    }

    private class SingleTypeFactory extends CircularTypeFactory {
        SingleTypeFactory(JsonNode schema, boolean onlyRequired) {
            super(List.of(schema), onlyRequired);
        }
    }

    @RequiredArgsConstructor
    private class CircularTypeFactory implements Supplier<JsonNode> {
        private final List<JsonNode> schemas;
        private final boolean onlyRequired;
        private int current = 0;

        @Override
        public JsonNode get() {
            var itemSchema = schemas.get(current++ % schemas.size());
            return tryMakeValueBySchema(itemSchema, onlyRequired);
        }
    }

    @RequiredArgsConstructor
    private static class CircularEnumFactory implements Supplier<JsonNode> {
        private final List<JsonNode> values;
        private int current = 0;

        @Override
        public JsonNode get() {
            return values.get(current++ % values.size());
        }
    }

    @Value(staticConstructor = "of")
    private static class Range {
        int min;
        int max;
    }
}
