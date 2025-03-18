package ru.yandex.ci.client.xiva;

import java.util.List;

import javax.annotation.Nonnull;

import com.fasterxml.jackson.annotation.JsonRawValue;
import com.fasterxml.jackson.databind.JsonNode;
import lombok.Value;

@Value
public class SendRequest {

    @JsonRawValue
    @Nonnull
    JsonNode payload;

    @Nonnull
    List<String> tags;

    public static SendRequest create(JsonNode payload) {
        return new SendRequest(payload, List.of());
    }

}
