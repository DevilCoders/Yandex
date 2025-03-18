package ru.yandex.ci.core.resolver.function;

import java.time.DateTimeException;
import java.time.Instant;
import java.time.ZoneId;
import java.time.ZoneOffset;
import java.time.ZonedDateTime;
import java.time.format.DateTimeFormatter;
import java.time.format.DateTimeParseException;
import java.util.List;
import java.util.Locale;

import io.burt.jmespath.Adapter;
import io.burt.jmespath.JmesPathType;
import io.burt.jmespath.function.ArgumentConstraints;
import io.burt.jmespath.function.BaseFunction;
import io.burt.jmespath.function.FunctionArgument;

import ru.yandex.ci.core.config.a.model.auto.ZoneIdDeserializer;

public class DateFormatFunc extends BaseFunction {

    private static final ZoneIdDeserializer ZONE_ID_DESERIALIZER = new ZoneIdDeserializer();

    public DateFormatFunc() {
        super(
                "date_format",
                ArgumentConstraints.typeOf(JmesPathType.STRING, JmesPathType.NUMBER),
                ArgumentConstraints.listOf(1, 2, ArgumentConstraints.typeOf(JmesPathType.STRING))
        );
    }

    @Override
    protected <T> T callFunction(Adapter<T> runtime, List<FunctionArgument<T>> arguments) {
        var valueArgument = arguments.get(0).value();
        var formatString = runtime.toString(arguments.get(1).value());

        var inputDate = runtime.typeOf(valueArgument) == JmesPathType.STRING
                ? parseStringValue(runtime.toString(valueArgument))
                : parseNumberValue(runtime.toNumber(valueArgument));

        var formatter = parseFormatString(formatString);
        if (arguments.size() >= 3) {
            inputDate = inputDate.withZoneSameInstant(parseZone(runtime.toString(arguments.get(2).value())));
        }
        try {
            return runtime.createString(formatter.format(inputDate));
        } catch (DateTimeException e) {
            throw new IllegalArgumentException(
                    "cannot represent value %s in format '%s': %s".formatted(
                            valueArgument.toString(), formatString, e.getMessage()
                    )
            );
        }
    }

    private ZonedDateTime parseNumberValue(Number epochSeconds) {
        try {
            var instant = Instant.ofEpochSecond(epochSeconds.intValue());
            return instant.atZone(ZoneOffset.UTC);
        } catch (DateTimeException e) {
            throw new IllegalArgumentException(
                    "cannot parse number value %s to instant: %s".formatted(epochSeconds, e.getMessage())
            );
        }
    }

    private ZoneId parseZone(String zoneId) {
        try {
            return ZONE_ID_DESERIALIZER.convert(zoneId);
        } catch (DateTimeException e) {
            throw new IllegalArgumentException(
                    "cannot parse timezone '%s': %s".formatted(
                            zoneId, e.getMessage()
                    )
            );
        }
    }

    private ZonedDateTime parseStringValue(String dateTimeString) {
        try {
            return ZonedDateTime.parse(dateTimeString);
        } catch (DateTimeParseException e) {
            throw new IllegalArgumentException(
                    "cannot parse value '%s' to instant: %s".formatted(dateTimeString, e.getMessage())
            );
        }
    }

    private DateTimeFormatter parseFormatString(String formatString) {
        try {
            return DateTimeFormatter.ofPattern(formatString, Locale.US);
        } catch (IllegalArgumentException e) {
            throw new IllegalArgumentException(
                    "cannot parse datetime format '%s': %s".formatted(formatString, e.getMessage())
            );
        }
    }
}
