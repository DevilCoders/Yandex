package ru.yandex.ci.util.gson;

import com.fasterxml.jackson.databind.util.StdConverter;
import com.google.gson.JsonElement;

/**
 * Yaml парсинг производится через Jackson, однако с json удобнее работать в Gson. Этот конвертер позволяет
 * десериализовать yaml узел в gson json объект.
 */
public class ToGsonElementDeserializer extends StdConverter<Object, JsonElement> {
    @Override
    public JsonElement convert(Object value) {
        return CiGson.instance().toJsonTree(value);
    }
}
