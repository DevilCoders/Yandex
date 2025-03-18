package ru.yandex.ci.client.charts.model.jackson;

import java.time.ZoneOffset;
import java.time.format.DateTimeFormatter;
import java.time.format.DateTimeFormatterBuilder;
import java.time.temporal.ChronoField;

import ru.yandex.ci.client.base.http.jackson.InstantSerializer;

public class ChartsInstantSerializer extends InstantSerializer {

    public static final DateTimeFormatter DATE_TIME_FORMATTER =
            new DateTimeFormatterBuilder()
                    .appendPattern("yyyy-MM-dd'T'HH:mm:ss")
                    .appendFraction(ChronoField.MICRO_OF_SECOND, 0, 6, true)
                    .appendZoneOrOffsetId()
                    .toFormatter()
                    .withZone(ZoneOffset.UTC);

    public ChartsInstantSerializer() {
        super(DATE_TIME_FORMATTER);
    }
}
