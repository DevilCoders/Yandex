package ru.yandex.ci.util.jackson;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;

import ru.yandex.ci.util.CiJson;

public class SerializationUtils {

    private SerializationUtils() {
        //
    }

    public static <T> T copy(T instance, Class<T> clazz) {
        return copy(instance, clazz, CiJson.mapper());
    }

    public static <T> T copy(T instance, Class<T> clazz, ObjectMapper mapper) {
        try {
            return mapper.readValue(mapper.writeValueAsString(instance), clazz);
        } catch (JsonProcessingException e) {
            throw new RuntimeException("Unable to copy object", e);
        }
    }
}
