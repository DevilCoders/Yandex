package ru.yandex.ci.core.config.a.model.auto;

import java.time.LocalTime;
import java.time.format.DateTimeFormatter;
import java.util.Locale;

import com.fasterxml.jackson.databind.util.StdConverter;

public class LocalTimeSimpleDeserializer extends StdConverter<String, LocalTime> {
    private static final DateTimeFormatter FORMATTER = DateTimeFormatter.ofPattern("H:mm", Locale.US);

    @Override
    public LocalTime convert(String value) {
        try {
            var hour = Integer.parseInt(value);
            if (hour >= 60) {
                return LocalTime.ofSecondOfDay(hour * 60L);
            }
            return LocalTime.of(hour, 0);
        } catch (NumberFormatException e) {
            return LocalTime.parse(value, FORMATTER);
        }
    }
}
