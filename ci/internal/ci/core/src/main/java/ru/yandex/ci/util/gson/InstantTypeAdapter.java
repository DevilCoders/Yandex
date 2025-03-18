package ru.yandex.ci.util.gson;

import java.lang.reflect.Type;
import java.time.Instant;

import com.google.gson.Gson;
import com.google.gson.JsonDeserializationContext;
import com.google.gson.JsonDeserializer;
import com.google.gson.JsonElement;
import com.google.gson.JsonParseException;

public class InstantTypeAdapter implements JsonDeserializer<Instant> {

    private static final Gson GSON = new Gson();

    @Override
    public Instant deserialize(JsonElement json, Type typeOfT,
                               JsonDeserializationContext context) throws JsonParseException {
        if (json.isJsonPrimitive()) {
            return Instant.parse(json.getAsString());
        }
        return GSON.fromJson(json, Instant.class);
    }
}
