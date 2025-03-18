package ru.yandex.ci.core.proto;

import java.lang.reflect.Type;
import java.time.Instant;

import com.google.gson.JsonDeserializationContext;
import com.google.gson.JsonDeserializer;
import com.google.gson.JsonElement;
import com.google.gson.JsonParseException;
import com.google.gson.JsonPrimitive;
import com.google.gson.JsonSerializationContext;
import com.google.gson.JsonSerializer;
import com.google.protobuf.Timestamp;

import ru.yandex.ci.common.proto.ProtoConverter;

public class ProtoTimestampAdapter implements JsonSerializer<Timestamp>, JsonDeserializer<Timestamp> {

    @Override
    public Timestamp deserialize(
            JsonElement json, Type typeOfT, JsonDeserializationContext context
    ) throws JsonParseException {
        return ProtoConverter.convert(Instant.parse(json.getAsString()));
    }

    @Override
    public JsonElement serialize(Timestamp src, Type typeOfSrc, JsonSerializationContext context) {
        return new JsonPrimitive(ProtoConverter.convert(src).toString());
    }
}
