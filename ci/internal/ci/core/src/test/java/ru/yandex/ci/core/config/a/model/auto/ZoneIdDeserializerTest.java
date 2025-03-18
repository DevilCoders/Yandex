package ru.yandex.ci.core.config.a.model.auto;

import java.time.ZoneId;
import java.time.ZoneOffset;
import java.util.stream.Stream;

import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.Arguments;
import org.junit.jupiter.params.provider.MethodSource;

import static org.assertj.core.api.Assertions.assertThat;
import static org.junit.jupiter.params.provider.Arguments.arguments;

class ZoneIdDeserializerTest {
    @ParameterizedTest
    @MethodSource
    void deserialize(String value, ZoneId expected) {
        var deserializer = new ZoneIdDeserializer();
        assertThat(deserializer.convert(value)).isEqualTo(expected);
    }

    static Stream<Arguments> deserialize() {
        return Stream.of(
                arguments("MSK", ZoneId.of("Europe/Moscow")),
                arguments("Europe/Moscow", ZoneId.of("Europe/Moscow")),
                arguments("Z", ZoneOffset.UTC),
                arguments("UTC", ZoneId.of("Etc/UTC")),
                arguments("GMT+4", ZoneOffset.ofHours(4)),
                arguments("GMT+4:30", ZoneOffset.ofHoursMinutes(4, 30)),
                arguments("KRAT", ZoneId.of("Asia/Krasnoyarsk")),
                arguments("IRKT", ZoneId.of("Asia/Irkutsk")),
                arguments("YEKT", ZoneId.of("Asia/Yekaterinburg"))

        );
    }
}
