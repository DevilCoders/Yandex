package ru.yandex.ci.core.config.a.model.auto;

import java.time.DayOfWeek;
import java.util.Set;
import java.util.stream.Collectors;

import com.fasterxml.jackson.databind.util.StdConverter;

public class DaysOfWeekRangeSerializer extends StdConverter<Set<DayOfWeek>, String> {

    @Override
    public String convert(Set<DayOfWeek> value) {
        return value.stream()
                .sorted()
                .map(DaysOfWeekRangeSerializer::renderDay)
                .collect(Collectors.joining(", "));
    }

    private static String renderDay(DayOfWeek day) {
        return switch (day) {
            case MONDAY -> "MON";
            case TUESDAY -> "TUE";
            case WEDNESDAY -> "WED";
            case THURSDAY -> "THU";
            case FRIDAY -> "FRI";
            case SATURDAY -> "SAT";
            case SUNDAY -> "SUN";
        };
    }
}
