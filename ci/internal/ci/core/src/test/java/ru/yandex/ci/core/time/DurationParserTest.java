package ru.yandex.ci.core.time;

import java.time.Duration;
import java.util.stream.Stream;

import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.Arguments;
import org.junit.jupiter.params.provider.MethodSource;

import static org.assertj.core.api.Assertions.assertThat;

class DurationParserTest {

    @ParameterizedTest
    @MethodSource("source")
    void parse(String value, Duration expected) {
        assertThat(DurationParser.parse(value)).isEqualTo(expected);
    }

    static Stream<Arguments> source() {
        return Stream.of(
                Arguments.of("3h", Duration.ofHours(3)),
                Arguments.of("3h 17m", Duration.ofHours(3).plusMinutes(17)),
                Arguments.of("5h 25s", Duration.ofHours(5).plusSeconds(25)),
                Arguments.of("3d 25s", Duration.ofDays(3).plusSeconds(25)),
                Arguments.of("4w 23s", Duration.ofDays(4 * 7).plusSeconds(23)),
                Arguments.of("4w 3d 23s", Duration.ofDays(4 * 7 + 3).plusSeconds(23))
        );
    }

}
