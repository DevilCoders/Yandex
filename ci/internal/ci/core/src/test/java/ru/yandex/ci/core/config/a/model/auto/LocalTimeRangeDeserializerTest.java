package ru.yandex.ci.core.config.a.model.auto;

import java.time.LocalTime;
import java.time.ZoneId;
import java.time.ZoneOffset;
import java.util.stream.Stream;

import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.Arguments;
import org.junit.jupiter.params.provider.MethodSource;

import static org.assertj.core.api.Assertions.assertThat;
import static org.junit.jupiter.params.provider.Arguments.arguments;

class LocalTimeRangeDeserializerTest {
    @ParameterizedTest
    @MethodSource
    void deserialize(String value, LocalTime start, LocalTime end, ZoneId zoneId) {
        var deserializer = new LocalTimeRangeDeserializer();
        assertThat(deserializer.convert(value)).isEqualTo(TimeRange.of(start, end, zoneId));
    }

    static Stream<Arguments> deserialize() {
        return Stream.of(
                arguments("19:30 - 20 MSK", time(19, 30), time(20, 0), zone("Europe/Moscow")),
                arguments("00:00 - 00:01 Europe/Moscow", time(0, 0), time(0, 1), zone("Europe/Moscow")),
                arguments("15-16 Z", time(15, 0), time(16, 0), ZoneOffset.UTC),
                arguments("00:00 - 05:00 Z", time(0, 0), time(5, 0), ZoneOffset.UTC),
                arguments("5:30 - 17:45 UTC", time(5, 30), time(17, 45), zone("Etc/UTC")),
                arguments("5:30-17:45 GMT+4", time(5, 30), time(17, 45), ZoneOffset.ofHours(4)),
                arguments("10-20 GMT+4:30", time(10, 0), time(20, 0), ZoneOffset.ofHoursMinutes(4, 30)),
                arguments("9 - 18 KRAT", time(9, 0), time(18, 0), zone("Asia/Krasnoyarsk")),
                arguments("10 - 20:00 IRKT", time(10, 0), time(20, 0), zone("Asia/Irkutsk")),
                arguments("9-17 YEKT", time(9, 0), time(17, 0), zone("Asia/Yekaterinburg"))

        );
    }

    private static ZoneId zone(String zone) {
        return ZoneId.of(zone);
    }

    private static LocalTime time(int hour, int minute) {
        return LocalTime.of(hour, minute);
    }
}
