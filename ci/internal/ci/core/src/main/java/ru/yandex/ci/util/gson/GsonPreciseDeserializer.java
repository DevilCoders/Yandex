package ru.yandex.ci.util.gson;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonPrimitive;
import com.google.gson.internal.LazilyParsedNumber;
import com.google.gson.internal.LinkedTreeMap;

/**
 * Direct deserializer, preserves original number type (does not convert every number into double)
 */
public class GsonPreciseDeserializer {

    private GsonPreciseDeserializer() {
        //
    }

    @SuppressWarnings("unchecked")
    public static Map<String, Object> toMap(JsonObject json) {
        var value = (Map<String, Object>) read(json);
        Preconditions.checkState(value != null, "Unable to convert json value to map, got null for %s", json);
        return value;
    }

    @Nullable
    private static Object read(JsonElement source) {
        if (source.isJsonPrimitive()) {
            JsonPrimitive primitive = source.getAsJsonPrimitive();
            // Сохраняет оригинальный тип переменной из JsonPrimitive
            if (primitive.isBoolean()) {
                return primitive.getAsBoolean();
            } else if (primitive.isString()) {
                return primitive.getAsString();
            } else if (primitive.isNumber()) {
                var number = primitive.getAsNumber();
                if (number instanceof LazilyParsedNumber lazyNumber) {
                    var toString = lazyNumber.toString();
                    if (toString.contains(".") || toString.contains(",")) {
                        return lazyNumber.doubleValue();
                    } else {
                        var longValue = lazyNumber.longValue();
                        if (longValue > Integer.MAX_VALUE || longValue < Integer.MIN_VALUE) {
                            return longValue;
                        } else {
                            return (int) longValue;
                        }
                    }
                } else {
                    return number;
                }
            } else if (primitive.isJsonNull()) {
                return null;
            } else {
                throw new IllegalArgumentException("unexpected primitive type: " + primitive);
            }
        } else if (source.isJsonNull()) {
            return null;
        } else if (source.isJsonArray()) {
            JsonArray sourceArray = source.getAsJsonArray();
            List<Object> targetArray = new ArrayList<>(sourceArray.size());
            for (JsonElement element : sourceArray) {
                targetArray.add(read(element));
            }
            return targetArray;
        } else if (source.isJsonObject()) {
            JsonObject sourceObject = source.getAsJsonObject();
            Map<String, Object> targetObject = new LinkedTreeMap<>();
            for (var entry : sourceObject.entrySet()) {
                targetObject.put(entry.getKey(), read(entry.getValue()));
            }
            return targetObject;
        }
        throw new IllegalArgumentException("unexpected node type: " + source);
    }
}
