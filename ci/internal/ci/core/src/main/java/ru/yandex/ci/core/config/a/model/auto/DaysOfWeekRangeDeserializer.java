package ru.yandex.ci.core.config.a.model.auto;

import java.time.DayOfWeek;
import java.util.Collection;
import java.util.Locale;
import java.util.Set;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import com.fasterxml.jackson.databind.util.StdConverter;

public class DaysOfWeekRangeDeserializer extends StdConverter<String, Set<DayOfWeek>> {
    @Override
    public Set<DayOfWeek> convert(String value) {
        var ranges = value.split(",");
        return Stream.of(ranges)
                .map(this::parseRange)
                .flatMap(Collection::stream)
                .collect(Collectors.toSet());
    }

    private Set<DayOfWeek> parseRange(String range) {
        var ends = range.split("-", 2);
        var from = parseDay(ends[0]);
        if (ends.length == 1) {
            return Set.of(from);
        }

        var to = parseDay(ends[1]);
        if (to.ordinal() < from.ordinal()) {
            throw new RuntimeException("failed to parse range '%s', begin of range should be after end"
                    .formatted(range));
        }

        return Stream.of(DayOfWeek.values())
                .filter(v -> v.ordinal() >= from.ordinal() && v.ordinal() <= to.ordinal())
                .collect(Collectors.toSet());
    }

    private static DayOfWeek parseDay(String dayStr) {
        return switch (dayStr.trim().toUpperCase(Locale.ROOT)) {
            case "MON" -> DayOfWeek.MONDAY;
            case "TUE" -> DayOfWeek.TUESDAY;
            case "WED" -> DayOfWeek.WEDNESDAY;
            case "THU" -> DayOfWeek.THURSDAY;
            case "FRI" -> DayOfWeek.FRIDAY;
            case "SAT" -> DayOfWeek.SATURDAY;
            case "SUN" -> DayOfWeek.SUNDAY;
            default -> throw new RuntimeException("cannot parse day of weak '" + dayStr + "'");
        };
    }
}
