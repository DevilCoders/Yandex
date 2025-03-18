package ru.yandex.ci.core.config.a.model.auto;

import java.time.LocalTime;
import java.util.stream.Stream;

import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.Arguments;
import org.junit.jupiter.params.provider.MethodSource;

import static org.assertj.core.api.Assertions.assertThat;
import static org.junit.jupiter.params.provider.Arguments.arguments;

class LocalTimeSimpleDeserializerTest {
    @ParameterizedTest
    @MethodSource
    void deserialize(String value, LocalTime expected) {
        var deserializer = new LocalTimeSimpleDeserializer();
        assertThat(deserializer.convert(value)).isEqualTo(expected);
    }

    static Stream<Arguments> deserialize() {
        return Stream.of(
                arguments("19:30", LocalTime.of(19, 30)),
                arguments("00:00", LocalTime.of(0, 0)),
                arguments("15", LocalTime.of(15, 0)),
                arguments(String.valueOf(60), LocalTime.of(1, 0)),
                arguments(String.valueOf(19 * 60 + 47), LocalTime.of(19, 47))
        );
    }

}
