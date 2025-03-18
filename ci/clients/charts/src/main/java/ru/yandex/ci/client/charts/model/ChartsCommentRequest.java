package ru.yandex.ci.client.charts.model;

import java.time.Instant;
import java.util.Map;

import javax.annotation.Nullable;

import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import lombok.Value;

import ru.yandex.ci.client.charts.model.jackson.ChartsInstantDeserializer;
import ru.yandex.ci.client.charts.model.jackson.ChartsInstantSerializer;

@Value
public class ChartsCommentRequest {

    @Nullable
    String feed;

    @Nullable
    ChartsCommentType type;

    @Nullable
    @JsonSerialize(using = ChartsInstantSerializer.class)
    @JsonDeserialize(using = ChartsInstantDeserializer.class)
    Instant date;

    @Nullable
    @JsonSerialize(using = ChartsInstantSerializer.class)
    @JsonDeserialize(using = ChartsInstantDeserializer.class)
    Instant dateUntil;

    @Nullable
    String text;

    @Nullable
    Map<String, Object> meta;

}
