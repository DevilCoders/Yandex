package ru.yandex.ci.engine.launch.auto;

import java.time.LocalDate;
import java.time.Month;
import java.time.ZonedDateTime;
import java.time.format.DateTimeFormatter;
import java.util.stream.Stream;

import com.fasterxml.jackson.core.JsonProcessingException;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.Arguments;
import org.junit.jupiter.params.provider.MethodSource;

import ru.yandex.ci.core.config.a.AYamlParser;
import ru.yandex.ci.core.config.a.model.auto.Schedule;
import ru.yandex.ci.flow.engine.definition.common.TypeOfSchedulerConstraint;
import ru.yandex.ci.flow.engine.runtime.calendar.TestWorkCalendarProvider;

import static org.assertj.core.api.Assertions.assertThat;
import static org.junit.jupiter.params.provider.Arguments.arguments;

class ScheduleCalculatorTest {
    private ScheduleCalculator calculator;
    private TestWorkCalendarProvider calendarProvider;

    @BeforeEach
    public void setUp() {
        calendarProvider = new TestWorkCalendarProvider();
        calculator = new ScheduleCalculator(calendarProvider);
    }

    @MethodSource
    @ParameterizedTest(name = "{1} for schedule {0} should be {2}")
    void contains(String schedule, String date, boolean contains) throws JsonProcessingException {
        var scheduleObject = parse(schedule);
        var dateTime = dateTime(date).toInstant();

        assertThat(calculator.contains(scheduleObject, dateTime))
                .isEqualTo(contains);
    }

    static Stream<Arguments> contains() {
        var schedule = """
                time: 9:00 - 19:30 GMT
                days: TUE-THU, Sat
                """;
        return Stream.of(
                arguments(schedule, "Mon, 2 Jun 2008 11:05:30 GMT", false),
                arguments(schedule, "Tue, 3 Jun 2008 11:05:30 GMT", true),
                arguments(schedule, "Wed, 4 Jun 2008 11:05:30 GMT", true),
                arguments(schedule, "Thu, 5 Jun 2008 11:05:30 GMT", true),
                arguments(schedule, "Fri, 6 Jun 2008 11:05:30 GMT", false),

                arguments(schedule, "Sat, 7 Jun 2008 8:59:59 GMT", false),
                arguments(schedule, "Sat, 7 Jun 2008 9:00:00 GMT", true),
                arguments(schedule, "Sat, 7 Jun 2008 11:05:30 GMT", true),
                arguments(schedule, "Sat, 7 Jun 2008 19:30:00 GMT", true),
                arguments(schedule, "Sat, 7 Jun 2008 19:30:01 GMT", false),

                arguments(schedule, "Sun, 8 Jun 2008 11:05:30 GMT", false)
        );
    }

    @MethodSource
    @ParameterizedTest(name = "{1} for schedule {0} should be {2}")
    void nextWindow(String schedule, String now, String next) throws JsonProcessingException {
        var scheduleObject = parse(schedule);
        var nowDateTime = dateTime(now).toInstant();
        var nextWindow = dateTime(next);

        assertThat(calculator.nextWindow(scheduleObject, nowDateTime))
                .isEqualTo(nextWindow);
    }

    static Stream<Arguments> nextWindow() {
        var schedule = """
                time: 9:00 - 19:30 GMT
                days: TUE-THU, Sat
                """;
        return Stream.of(
                arguments(schedule, "Mon, 2 Jun 2008 11:05:30 GMT", "Tue, 3 Jun 2008 09:00:00 GMT"),
                arguments(schedule, "Tue, 3 Jun 2008 11:05:30 GMT", "Tue, 3 Jun 2008 11:05:30 GMT"),
                arguments(schedule, "Fri, 6 Jun 2008 11:05:30 GMT", "Sat, 7 Jun 2008 09:00:00 GMT"),

                arguments(schedule, "Sat, 7 Jun 2008 8:59:59 GMT", "Sat, 7 Jun 2008 09:00:00 GMT"),
                arguments(schedule, "Sat, 7 Jun 2008 9:00:00 GMT", "Sat, 7 Jun 2008 09:00:00 GMT"),
                arguments(schedule, "Sat, 7 Jun 2008 11:05:30 GMT", "Sat, 7 Jun 2008 11:05:30 GMT"),
                arguments(schedule, "Sat, 7 Jun 2008 19:30:00 GMT", "Sat, 7 Jun 2008 19:30:00 GMT"),
                arguments(schedule, "Sat, 7 Jun 2008 19:30:01 GMT", "Tue, 10 Jun 2008 09:00:00 GMT"),

                arguments(schedule, "Sun, 8 Jun 2008 11:05:30 GMT", "Tue, 10 Jun 2008 09:00:00 GMT")
        );
    }

    @Test
    void dayType() throws JsonProcessingException {
        var schedule = parse("""
                time: 9:00 - 19:30 GMT
                days: MON
                day-type: not-pre-holidays
                """);

        var now = dateTime("Mon, 2 Jun 2008 11:05:30 GMT");
        var nowInstant = now.toInstant();

        assertThat(calculator.nextWindow(schedule, nowInstant)).isEqualTo(now);

        calendarProvider.setDayType(LocalDate.of(2008, Month.JUNE, 2), TypeOfSchedulerConstraint.HOLIDAY);
        calendarProvider.setDayType(LocalDate.of(2008, Month.JUNE, 9), TypeOfSchedulerConstraint.PRE_HOLIDAY);
        assertThat(calculator.nextWindow(schedule, nowInstant)).isEqualTo(dateTime("Mon, 16 Jun 2008 9:00:00 GMT"));
    }

    @Test
    void workdays() throws JsonProcessingException {
        var schedule = parse("""
                days: MON-SUN
                day-type: workdays
                time: 04:00 - 04:30 MSK
                 """);

        var now = dateTime("Sat, 06 Nov 2021 11:06:12 +0300");

        calendarProvider.setDayType(LocalDate.of(2021, Month.NOVEMBER, 6), TypeOfSchedulerConstraint.HOLIDAY);
        calendarProvider.setDayType(LocalDate.of(2021, Month.NOVEMBER, 7), TypeOfSchedulerConstraint.HOLIDAY);

        assertThat(calculator.nextWindow(schedule, now.toInstant()))
                .isEqualTo(dateTime("Mon, 08 Nov 2021 04:00:00 +0300"));
    }

    @Test
    void timeOnly() throws JsonProcessingException {
        var schedule = parse("""
                time: 9:00 - 19:30 GMT
                """);

        assertThat(calculator.nextWindow(schedule, dateTime("Mon, 2 Jun 2008 11:05:30 GMT").toInstant()))
                .isEqualTo(dateTime("Mon, 2 Jun 2008 11:05:30 GMT"));

        assertThat(calculator.nextWindow(schedule, dateTime("Mon, 2 Jun 2008 19:31:00 GMT").toInstant()))
                .isEqualTo(dateTime("Tue, 3 Jun 2008 09:00:00 GMT"));
    }

    @Test
    void dayTypeWithoutDays() throws JsonProcessingException {
        var schedule = parse("""
                time: 9:00 - 19:30 GMT
                day-type: not-pre-holidays
                """);

        assertThat(calculator.nextWindow(schedule, dateTime("Fri, 6 Jun 2008 19:31:00 GMT").toInstant()))
                .isEqualTo(dateTime("Mon, 9 Jun 2008 09:00:00 GMT"));
    }

    private static Schedule parse(String yamlChunk) throws JsonProcessingException {
        return AYamlParser.getMapper().readValue(yamlChunk, Schedule.class);
    }

    private static ZonedDateTime dateTime(String s) {
        return ZonedDateTime.parse(s, DateTimeFormatter.RFC_1123_DATE_TIME);
    }
}
