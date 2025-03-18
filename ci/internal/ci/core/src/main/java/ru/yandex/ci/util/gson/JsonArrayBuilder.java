package ru.yandex.ci.util.gson;

import java.util.function.Function;

import com.google.gson.JsonArray;
import com.google.gson.JsonElement;

import ru.yandex.ci.util.NestedBuilder;

public class JsonArrayBuilder<Parent> extends NestedBuilder<Parent, JsonArray> {
    private final JsonArray array = new JsonArray();

    public JsonArrayBuilder(Function<JsonArray, Parent> toParent) {
        super(toParent);
    }

    public static JsonArrayBuilder<?> builder() {
        return new JsonArrayBuilder<>(Function.identity());
    }

    public JsonArrayBuilder<Parent> withValue(String value) {
        array.add(value);
        return this;
    }

    public JsonArrayBuilder<Parent> withValues(String... values) {
        for (String value : values) {
            array.add(value);
        }
        return this;
    }

    public JsonArrayBuilder<Parent> withValues(JsonElement... values) {
        for (JsonElement value : values) {
            array.add(value);
        }
        return this;
    }

    public JsonArrayBuilder<Parent> withValue(Number value) {
        array.add(value);
        return this;
    }

    public JsonArrayBuilder<Parent> withValue(Boolean value) {
        array.add(value);
        return this;
    }

    public JsonArrayBuilder<Parent> withValue(JsonElement value) {
        array.add(value);
        return this;
    }

    public JsonArrayBuilder<JsonArrayBuilder<Parent>> startArray() {
        return new JsonArrayBuilder<>(this::withValue);
    }

    public JsonObjectBuilder<JsonArrayBuilder<Parent>> startObject() {
        return new JsonObjectBuilder<>(this::withValue);
    }

    @Override
    public JsonArray build() {
        return array.deepCopy();
    }
}
