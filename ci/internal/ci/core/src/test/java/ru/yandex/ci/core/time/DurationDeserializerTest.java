package ru.yandex.ci.core.time;

import java.time.Duration;
import java.util.stream.Stream;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.Arguments;
import org.junit.jupiter.params.provider.MethodSource;

import ru.yandex.ci.core.config.a.model.DurationSerializer;

import static org.assertj.core.api.Assertions.assertThat;
import static org.assertj.core.api.Assertions.assertThatThrownBy;

class DurationDeserializerTest {
    DurationDeserializer deserializer;
    DurationSerializer serializer;

    @BeforeEach
    public void setUp() {
        deserializer = new DurationDeserializer();
        serializer = new DurationSerializer();
    }

    @ParameterizedTest
    @MethodSource("source")
    void parse(String value, Duration expected) {
        assertThat(deserializer.convert(value)).isEqualTo(expected);
    }

    @ParameterizedTest
    @MethodSource("source")
    void serialize(String expected, Duration value) {
        assertThat(serializer.convert(value)).isEqualTo(expected);
    }

    @Test
    void serializeZero() {
        assertThatThrownBy(() -> serializer.convert(Duration.ZERO))
                .isInstanceOf(IllegalArgumentException.class)
                .hasMessage("zero duration is not supported: 'PT0S'");
    }

    @Test
    void deserializeZero() {
        assertThatThrownBy(() -> deserializer.convert("0"))
                .isInstanceOf(IllegalArgumentException.class)
                .hasMessage("zero duration is not supported: '0'");
        assertThatThrownBy(() -> deserializer.convert("0s"))
                .isInstanceOf(IllegalArgumentException.class)
                .hasMessage("zero duration is not supported: '0s'");
    }

    static Stream<Arguments> source() {
        return DurationParserTest.source();
    }
}
