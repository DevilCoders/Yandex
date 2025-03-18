package ru.yandex.ci.util.gson;

import java.util.function.Function;

import com.google.gson.JsonElement;
import com.google.gson.JsonObject;

import ru.yandex.ci.util.NestedBuilder;

public class JsonObjectBuilder<Parent> extends NestedBuilder<Parent, JsonObject> {
    private final JsonObject object = new JsonObject();

    public JsonObjectBuilder(Function<JsonObject, Parent> toParent) {
        super(toParent);
    }

    public JsonObjectBuilder<Parent> withProperty(String key, String value) {
        object.addProperty(key, value);
        return this;
    }

    public JsonObjectBuilder<Parent> withProperty(String key, Number value) {
        object.addProperty(key, value);
        return this;
    }

    public JsonObjectBuilder<Parent> withProperty(String key, Boolean value) {
        object.addProperty(key, value);
        return this;
    }

    public JsonObjectBuilder<Parent> withProperty(String key, JsonElement value) {
        object.add(key, value);
        return this;
    }

    public JsonObjectBuilder<JsonObjectBuilder<Parent>> startMap(String key) {
        return new JsonObjectBuilder<>(object -> withProperty(key, object));
    }

    public JsonArrayBuilder<JsonObjectBuilder<Parent>> startArray(String key) {
        return new JsonArrayBuilder<>(array -> withProperty(key, array));
    }

    public static JsonObjectBuilder<?> builder() {
        return new JsonObjectBuilder<>(Function.identity());
    }

    @Override
    public JsonObject build() {
        return object.deepCopy();
    }
}
