package ru.yandex.ci.client.sandbox;

import java.time.OffsetDateTime;
import java.time.format.DateTimeFormatter;
import java.time.format.DateTimeFormatterBuilder;
import java.time.temporal.ChronoField;

public class SandboxDateTimeFormatter {

    private static final DateTimeFormatter DATE_TIME_FORMATTER =
            new DateTimeFormatterBuilder()
                    .appendPattern("yyyy-MM-dd'T'HH:mm:ss")
                    .appendFraction(ChronoField.MICRO_OF_SECOND, 0, 6, true)
                    .appendZoneOrOffsetId()
                    .toFormatter();


    private SandboxDateTimeFormatter() {
    }

    public static String format(OffsetDateTime from, OffsetDateTime to) {
        return DATE_TIME_FORMATTER.format(from) + ".." + DATE_TIME_FORMATTER.format(to);
    }

    public static String format(OffsetDateTime dateTime) {
        return DATE_TIME_FORMATTER.format(dateTime);
    }
}
