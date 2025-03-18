package ru.yandex.ci.core.config.a.model;

import java.time.Duration;

import com.fasterxml.jackson.databind.util.StdConverter;

public class DurationSerializer extends StdConverter<Duration, String> {
    @Override
    public String convert(Duration value) {
        if (value.isZero()) {
            throw new IllegalArgumentException("zero duration is not supported: '" + value + "'");
        }
        StringBuilder sb = new StringBuilder("xw xxd xxh xxm xxs".length());
        var days = value.toDaysPart();
        if (days >= 7) {
            var weeks = days / 7;
            sb.append(weeks).append("w");
            value = value.minusDays(weeks * 7);
        }
        addPart(value.toDaysPart(), sb, "d");
        addPart(value.toHoursPart(), sb, "h");
        addPart(value.toMinutesPart(), sb, "m");
        addPart(value.toSecondsPart(), sb, "s");
        return sb.toString();
    }

    private static void addPart(long value, StringBuilder sb, String suffix) {
        if (value == 0) {
            return;
        }
        if (!sb.isEmpty()) {
            sb.append(' ');
        }
        sb.append(value).append(suffix);
    }
}
