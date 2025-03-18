package ru.yandex.ci.core.config.a.model.auto;

import java.time.DayOfWeek;
import java.util.Set;
import java.util.stream.Stream;

import org.assertj.core.util.Sets;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.Arguments;
import org.junit.jupiter.params.provider.MethodSource;

import static java.time.DayOfWeek.FRIDAY;
import static java.time.DayOfWeek.MONDAY;
import static java.time.DayOfWeek.SATURDAY;
import static java.time.DayOfWeek.SUNDAY;
import static java.time.DayOfWeek.THURSDAY;
import static java.time.DayOfWeek.TUESDAY;
import static java.time.DayOfWeek.WEDNESDAY;
import static org.assertj.core.api.Assertions.assertThat;
import static org.junit.jupiter.params.provider.Arguments.arguments;

class DaysOfWeekRangeDeserializerTest {
    private DaysOfWeekRangeDeserializer deserializer;

    @BeforeEach
    void setUp() {
        deserializer = new DaysOfWeekRangeDeserializer();
    }

    @ParameterizedTest(name = "\"{0}\" -> {1}")
    @MethodSource
    void deserializeTest(String range, Set<DayOfWeek> days) {
        assertThat(deserializer.convert(range))
                .isEqualTo(days);
    }

    static Stream<Arguments> deserializeTest() {
        return Stream.of(
                arguments("MON", Sets.newLinkedHashSet(MONDAY)),
                arguments("TUE", Sets.newLinkedHashSet(TUESDAY)),
                arguments("WED", Sets.newLinkedHashSet(WEDNESDAY)),
                arguments("THU", Sets.newLinkedHashSet(THURSDAY)),
                arguments("FRI", Sets.newLinkedHashSet(FRIDAY)),
                arguments("SAT", Sets.newLinkedHashSet(SATURDAY)),
                arguments("SUN", Sets.newLinkedHashSet(SUNDAY)),
                arguments("MON-THU", Sets.newLinkedHashSet(MONDAY, TUESDAY, WEDNESDAY, THURSDAY)),
                arguments("TUE, FRI", Sets.newLinkedHashSet(TUESDAY, FRIDAY)),
                arguments("MON, WED-FRI", Sets.newLinkedHashSet(MONDAY, WEDNESDAY, THURSDAY, FRIDAY))
        );
    }
}
