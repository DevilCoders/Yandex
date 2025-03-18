package ru.yandex.ci.util.gson;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.util.StdConverter;
import com.google.gson.JsonElement;

/**
 * Yaml парсинг производится через Jackson, однако с json удобнее работать в Gson. Этот конвертер позволяет
 * десериализовать yaml узел в gson json объект.
 */
public class GsonElementSerializer extends StdConverter<JsonElement, JsonNode> {
    private final ObjectMapper mapper = new ObjectMapper();

    @Override
    public JsonNode convert(JsonElement value) {
        try {
            String json = CiGson.instance().toJson(value);
            return mapper.readTree(json);
        } catch (JsonProcessingException e) {
            throw new RuntimeException(e);
        }
    }
}
