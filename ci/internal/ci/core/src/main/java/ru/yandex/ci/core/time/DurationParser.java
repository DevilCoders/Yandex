package ru.yandex.ci.core.time;

import java.time.Duration;
import java.util.function.Function;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class DurationParser {
    private static final Pattern PATTERN = Pattern.compile(
            "^((?<week>[0-9]) *w)? *" +
                    "((?<day>[0-9]+) *d)? *" +
                    "((?<hour>[0-9]+) *h)? *" +
                    "((?<minute>[0-9]+) *m)? *" +
                    "((?<second>[0-9]+) *s)?$" +
                    "|^0+$"
    );

    private DurationParser() {
    }

    public static Duration parse(String value) {
        var matcher = PATTERN.matcher(value);
        if (!matcher.matches()) {
            throw new IllegalArgumentException("wrong duration: '" + value + "'");
        }
        var duration = Duration.ZERO;
        duration = plus(duration, matcher, "week", val -> Duration.ofDays(val * 7));
        duration = plus(duration, matcher, "day", Duration::ofDays);
        duration = plus(duration, matcher, "hour", Duration::ofHours);
        duration = plus(duration, matcher, "minute", Duration::ofMinutes);
        duration = plus(duration, matcher, "second", Duration::ofSeconds);

        if (duration.isZero()) {
            throw new IllegalArgumentException("zero duration is not supported: '" + value + "'");
        }

        return duration;
    }

    private static Duration plus(Duration current, Matcher matcher, String group, Function<Long, Duration> adder) {
        var stringValue = matcher.group(group);
        if (stringValue == null) {
            return current;
        }
        return current.plus(adder.apply(Long.parseLong(stringValue)));
    }
}
