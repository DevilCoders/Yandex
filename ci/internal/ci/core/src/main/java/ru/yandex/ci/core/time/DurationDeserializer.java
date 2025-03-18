package ru.yandex.ci.core.time;

import java.time.Duration;

import com.fasterxml.jackson.databind.util.StdConverter;

public class DurationDeserializer extends StdConverter<String, Duration> {

    @Override
    public Duration convert(String value) {
        return DurationParser.parse(value);
    }

}
