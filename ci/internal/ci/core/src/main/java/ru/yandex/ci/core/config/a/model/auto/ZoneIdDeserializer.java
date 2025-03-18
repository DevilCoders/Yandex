package ru.yandex.ci.core.config.a.model.auto;

import java.time.ZoneId;
import java.time.format.DateTimeFormatter;
import java.time.format.DateTimeParseException;

import com.fasterxml.jackson.databind.util.StdConverter;

public class ZoneIdDeserializer extends StdConverter<String, ZoneId> {
    private static final DateTimeFormatter TIMEZONE_NAME_FORMAT = DateTimeFormatter.ofPattern("z");
    private static final DateTimeFormatter OFFSET_FORMAT = DateTimeFormatter.ofPattern("O");

    @Override
    public ZoneId convert(String value) {
        try {
            return ZoneId.from(TIMEZONE_NAME_FORMAT.parse(value));
        } catch (DateTimeParseException e) {
            return ZoneId.from(OFFSET_FORMAT.parse(value));
        }
    }
}
