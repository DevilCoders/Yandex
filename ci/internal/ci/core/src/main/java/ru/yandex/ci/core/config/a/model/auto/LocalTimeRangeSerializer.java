package ru.yandex.ci.core.config.a.model.auto;

import com.fasterxml.jackson.databind.util.StdConverter;

public class LocalTimeRangeSerializer extends StdConverter<TimeRange, String> {

    @Override
    public String convert(TimeRange value) {
        return value.getStart() + " - " + value.getEnd() + " " + value.getZoneId();
    }
}
