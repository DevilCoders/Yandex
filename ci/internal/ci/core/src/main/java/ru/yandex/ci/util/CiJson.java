package ru.yandex.ci.util;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;

import yandex.cloud.util.Json;
import ru.yandex.ci.util.gson.GsonToJacksonMapperModule;

public class CiJson {

    private static final GsonToJacksonMapperModule MAPPER_MODULE = new GsonToJacksonMapperModule();

    static {
        init();
    }

    private CiJson() {
        //
    }

    public static void init() {
        Json.mapper.registerModule(MAPPER_MODULE);
    }

    public static ObjectMapper mapper() {
        return Json.mapper;
    }

    public static JsonNode readTree(Object object) {
        return readTree(writeValueAsString(object));
    }

    public static JsonNode readTree(String json) {
        try {
            return mapper().readTree(json);
        } catch (JsonProcessingException e) {
            throw new RuntimeException(e);
        }
    }

    public static <T> T readValue(String json, Class<T> clazz) {
        try {
            return mapper().readValue(json, clazz);
        } catch (JsonProcessingException e) {
            throw new RuntimeException(e);
        }
    }

    public static String writeValueAsString(Object value) {
        try {
            return mapper().writeValueAsString(value);
        } catch (JsonProcessingException e) {
            throw new RuntimeException(e);
        }
    }
}
