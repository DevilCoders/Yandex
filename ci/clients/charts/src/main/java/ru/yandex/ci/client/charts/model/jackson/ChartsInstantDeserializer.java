package ru.yandex.ci.client.charts.model.jackson;

import ru.yandex.ci.client.base.http.jackson.InstantDeserializer;

public class ChartsInstantDeserializer extends InstantDeserializer {
    public ChartsInstantDeserializer() {
        super(ChartsInstantSerializer.DATE_TIME_FORMATTER);
    }
}
