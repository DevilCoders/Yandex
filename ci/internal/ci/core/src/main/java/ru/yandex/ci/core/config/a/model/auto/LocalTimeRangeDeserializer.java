package ru.yandex.ci.core.config.a.model.auto;

import java.time.LocalTime;
import java.time.ZoneId;

import com.fasterxml.jackson.databind.util.StdConverter;

public class LocalTimeRangeDeserializer extends StdConverter<String, TimeRange> {
    @Override
    public TimeRange convert(String value) {
        var values = value.split("-", 2);
        var timeAndZone = values[1].trim().split("\s", 2);
        var start = parseTime(values[0]);
        var end = parseTime(timeAndZone[0]);
        var zoneId = parseZoneId(timeAndZone[1]);

        return TimeRange.of(start, end, zoneId);
    }

    private ZoneId parseZoneId(String value) {
        var deserializer = new ZoneIdDeserializer();
        return deserializer.convert(value);
    }

    private LocalTime parseTime(String value) {
        var deserializer = new LocalTimeSimpleDeserializer();
        return deserializer.convert(value.trim());
    }


}
