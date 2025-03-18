package ru.yandex.ci.util.gson;

import java.nio.file.Path;
import java.time.Instant;
import java.time.LocalDate;
import java.time.LocalDateTime;
import java.util.Map;

import com.google.common.base.CaseFormat;
import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.JsonObject;
import com.google.gson.JsonPrimitive;
import com.google.gson.JsonSerializer;
import com.google.protobuf.Message;
import com.google.protobuf.StringValue;
import com.google.protobuf.Timestamp;

import ru.yandex.ci.core.proto.ProtoTimestampAdapter;
import ru.yandex.ci.core.proto.ProtoTypeAdapter;

public class CiGson {
    private static final Gson GSON;

    static {
        var adapter = ProtoTypeAdapter.newBuilder()
                .setFieldNameSerializationFormat(CaseFormat.LOWER_UNDERSCORE, CaseFormat.LOWER_UNDERSCORE)
                .build();
        GSON = builder(adapter).serializeNulls().create();
    }

    private CiGson() {
        //
    }

    public static GsonBuilder builder(ProtoTypeAdapter adapter) {
        return new GsonBuilder()
                .registerTypeAdapter(LocalDateTime.class, new LocalDateTimeAdapter())
                .registerTypeAdapter(LocalDate.class, new LocalDateAdapter())
                .registerTypeHierarchyAdapter(Path.class, new PathTypeAdapter())
                .registerTypeAdapter(Instant.class, new InstantTypeAdapter())
                .registerTypeAdapter(Timestamp.class, new ProtoTimestampAdapter())
                .enableComplexMapKeySerialization()
                .registerTypeHierarchyAdapter(Message.class, adapter)
                .registerTypeHierarchyAdapter(StringValue.class,
                        (JsonSerializer<Object>) (src, typeOfSrc, context) ->
                                new JsonPrimitive(((StringValue) src).getValue()));
    }

    public static Gson instance() {
        return GSON;
    }

    // This method is somewhat different from standard GSON deserializer - it preserve primitive types
    // All of this must be removed after CI-1584
    public static Map<String, Object> toMap(JsonObject json) {
        return GsonPreciseDeserializer.toMap(json);
    }
}
