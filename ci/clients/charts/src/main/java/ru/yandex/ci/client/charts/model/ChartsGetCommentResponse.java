package ru.yandex.ci.client.charts.model;

import java.time.Instant;

import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import lombok.Value;

import ru.yandex.ci.client.charts.model.jackson.ChartsInstantDeserializer;
import ru.yandex.ci.client.charts.model.jackson.ChartsInstantSerializer;

@Value
public class ChartsGetCommentResponse {
    String id;
    String feed;
    String creatorLogin;

    @JsonSerialize(using = ChartsInstantSerializer.class)
    @JsonDeserialize(using = ChartsInstantDeserializer.class)
    Instant date;

    @JsonSerialize(using = ChartsInstantSerializer.class)
    @JsonDeserialize(using = ChartsInstantDeserializer.class)
    Instant dateUntil;

}
