package ru.yandex.ci.core.proto;

import java.util.Map;

import com.fasterxml.jackson.databind.JsonNode;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.protobuf.InvalidProtocolBufferException;
import com.google.protobuf.Message;
import com.google.protobuf.util.JsonFormat;

import ru.yandex.ci.util.CiJson;
import ru.yandex.ci.util.gson.CiGson;

public class ProtobufSerialization {

    private ProtobufSerialization() {
        //
    }

//    public static JsonObject serializeToGson(Message message) {
//        return JsonParser.parseString(serializeToJsonString(message)).getAsJsonObject();
//    }

    // TODO: cannot switch to to standard serialization due to compatibility reasons
    public static JsonObject serializeToGson(Message message) {
        return (JsonObject) CiGson.instance().toJsonTree(message);
    }

    public static Map<String, ?> serializeToGsonMap(Message message) {
        return CiGson.toMap(serializeToGson(message));
    }

    public static JsonNode serializeToJson(Message message) {
        return CiJson.readTree(serializeToJsonString(message));
    }

    public static String serializeToJsonString(Message message) {
        try {
            return JsonFormat.printer()
                    .includingDefaultValueFields()
                    .preservingProtoFieldNames()
                    .print(message);
        } catch (InvalidProtocolBufferException e) {
            throw new RuntimeException("Unable to serialize message as JSON", e);
        }
    }

    //

    public static <T extends Message.Builder> T deserializeFromGson(JsonElement json, T builder)
            throws InvalidProtocolBufferException {
        return deserializeFromJsonString(json.toString(), builder);
    }

    public static <T extends Message.Builder> T deserializeFromJsonString(String json, T builder)
            throws InvalidProtocolBufferException {
        JsonFormat.parser().merge(json, builder);
        return builder;
    }

}
