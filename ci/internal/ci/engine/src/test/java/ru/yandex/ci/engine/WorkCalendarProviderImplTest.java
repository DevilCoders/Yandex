package ru.yandex.ci.engine;

import java.time.LocalDate;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.mockito.Mock;
import org.mockito.junit.jupiter.MockitoExtension;

import ru.yandex.ci.client.calendar.CalendarClient;
import ru.yandex.ci.client.calendar.api.Country;
import ru.yandex.ci.client.calendar.api.DayType;
import ru.yandex.ci.client.calendar.api.Holiday;
import ru.yandex.ci.client.calendar.api.Holidays;
import ru.yandex.ci.flow.engine.definition.common.TypeOfSchedulerConstraint;
import ru.yandex.ci.flow.engine.runtime.calendar.WorkCalendarProviderException;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.lenient;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

@ExtendWith(MockitoExtension.class)
class WorkCalendarProviderImplTest {
    private WorkCalendarProviderImpl provider;

    @Mock
    private CalendarClient calendarClient;

    @BeforeEach
    public void setUp() {
        provider = new WorkCalendarProviderImpl(calendarClient);

        lenient().when(calendarClient.getHolidays(LocalDate.of(2007, 5, 1), LocalDate.of(2007, 6, 1), Country.RUSSIA))
                .thenReturn(Holidays.of(
                        holiday(LocalDate.of(2007, 5, 1), DayType.WEEKDAY),
                        holiday(LocalDate.of(2007, 5, 2), DayType.WEEKDAY),
                        holiday(LocalDate.of(2007, 5, 3), DayType.WEEKDAY),
                        holiday(LocalDate.of(2007, 5, 4), DayType.WEEKDAY),
                        holiday(LocalDate.of(2007, 5, 5), DayType.WEEKDAY),
                        holiday(LocalDate.of(2007, 5, 6), DayType.WEEKEND),
                        holiday(LocalDate.of(2007, 5, 7), DayType.WEEKEND),
                        holiday(LocalDate.of(2007, 5, 8), DayType.WEEKDAY),
                        holiday(LocalDate.of(2007, 5, 9), DayType.WEEKDAY),
                        holiday(LocalDate.of(2007, 5, 10), DayType.HOLIDAY),
                        holiday(LocalDate.of(2007, 5, 11), DayType.WEEKDAY),
                        holiday(LocalDate.of(2007, 5, 12), DayType.WEEKDAY),
                        holiday(LocalDate.of(2007, 5, 13), DayType.WEEKEND),
                        holiday(LocalDate.of(2007, 5, 14), DayType.WEEKEND),
                        holiday(LocalDate.of(2007, 5, 15), DayType.WEEKDAY),
                        holiday(LocalDate.of(2007, 5, 16), DayType.WEEKDAY),
                        holiday(LocalDate.of(2007, 5, 17), DayType.WEEKDAY),
                        holiday(LocalDate.of(2007, 5, 18), DayType.WEEKDAY),
                        holiday(LocalDate.of(2007, 5, 19), DayType.WEEKDAY),
                        holiday(LocalDate.of(2007, 5, 20), DayType.WEEKEND),
                        holiday(LocalDate.of(2007, 5, 21), DayType.WEEKEND),
                        holiday(LocalDate.of(2007, 5, 22), DayType.WEEKDAY),
                        holiday(LocalDate.of(2007, 5, 23), DayType.WEEKDAY),
                        holiday(LocalDate.of(2007, 5, 24), DayType.WEEKDAY),
                        holiday(LocalDate.of(2007, 5, 25), DayType.WEEKDAY),
                        holiday(LocalDate.of(2007, 5, 26), DayType.WEEKDAY),
                        holiday(LocalDate.of(2007, 5, 27), DayType.WEEKEND),
                        holiday(LocalDate.of(2007, 5, 28), DayType.WEEKEND),
                        holiday(LocalDate.of(2007, 5, 29), DayType.WEEKDAY),
                        holiday(LocalDate.of(2007, 5, 30), DayType.WEEKDAY),
                        holiday(LocalDate.of(2007, 5, 31), DayType.WEEKDAY),
                        holiday(LocalDate.of(2007, 6, 1), DayType.WEEKDAY)
                ));
        lenient().when(calendarClient.getHolidays(LocalDate.of(2007, 6, 1), LocalDate.of(2007, 7, 1), Country.RUSSIA))
                .thenReturn(Holidays.of(
                        holiday(LocalDate.of(2007, 6, 1), DayType.WEEKDAY),
                        holiday(LocalDate.of(2007, 6, 2), DayType.HOLIDAY),
                        holiday(LocalDate.of(2007, 6, 3), DayType.WEEKDAY),
                        holiday(LocalDate.of(2007, 6, 4), DayType.WEEKDAY),
                        holiday(LocalDate.of(2007, 6, 5), DayType.WEEKDAY),
                        holiday(LocalDate.of(2007, 6, 6), DayType.WEEKEND),
                        holiday(LocalDate.of(2007, 6, 7), DayType.WEEKEND),
                        holiday(LocalDate.of(2007, 6, 8), DayType.WEEKDAY),
                        holiday(LocalDate.of(2007, 6, 9), DayType.WEEKDAY),
                        holiday(LocalDate.of(2007, 6, 10), DayType.HOLIDAY),
                        holiday(LocalDate.of(2007, 6, 11), DayType.WEEKDAY),
                        holiday(LocalDate.of(2007, 6, 12), DayType.WEEKDAY),
                        holiday(LocalDate.of(2007, 6, 13), DayType.WEEKEND),
                        holiday(LocalDate.of(2007, 6, 14), DayType.WEEKEND),
                        holiday(LocalDate.of(2007, 6, 15), DayType.WEEKDAY),
                        holiday(LocalDate.of(2007, 6, 16), DayType.WEEKDAY),
                        holiday(LocalDate.of(2007, 6, 17), DayType.WEEKDAY),
                        holiday(LocalDate.of(2007, 6, 18), DayType.WEEKDAY),
                        holiday(LocalDate.of(2007, 6, 19), DayType.WEEKDAY),
                        holiday(LocalDate.of(2007, 6, 20), DayType.WEEKEND),
                        holiday(LocalDate.of(2007, 6, 21), DayType.WEEKEND),
                        holiday(LocalDate.of(2007, 6, 22), DayType.WEEKDAY),
                        holiday(LocalDate.of(2007, 6, 23), DayType.WEEKDAY),
                        holiday(LocalDate.of(2007, 6, 24), DayType.WEEKDAY),
                        holiday(LocalDate.of(2007, 6, 25), DayType.WEEKDAY),
                        holiday(LocalDate.of(2007, 6, 26), DayType.WEEKDAY),
                        holiday(LocalDate.of(2007, 6, 27), DayType.WEEKEND),
                        holiday(LocalDate.of(2007, 6, 28), DayType.WEEKEND),
                        holiday(LocalDate.of(2007, 6, 29), DayType.WEEKDAY),
                        holiday(LocalDate.of(2007, 6, 30), DayType.WEEKDAY),
                        holiday(LocalDate.of(2007, 7, 1), DayType.WEEKDAY)
                ));
    }

    @Test
    void cache() throws WorkCalendarProviderException {
        provider.getTypeOfDay(LocalDate.of(2007, 5, 27));
        provider.getTypeOfDay(LocalDate.of(2007, 5, 25));
        provider.getTypeOfDay(LocalDate.of(2007, 5, 27));
        provider.getTypeOfDay(LocalDate.of(2007, 5, 23));

        verify(calendarClient, times(1)).getHolidays(any(), any(), any());
    }

    @Test
    void simple() throws WorkCalendarProviderException {
        assertThat(provider.getTypeOfDay(LocalDate.of(2007, 5, 8))).isEqualTo(TypeOfSchedulerConstraint.WORK);
        assertThat(provider.getTypeOfDay(LocalDate.of(2007, 5, 9))).isEqualTo(TypeOfSchedulerConstraint.PRE_HOLIDAY);
        assertThat(provider.getTypeOfDay(LocalDate.of(2007, 5, 10))).isEqualTo(TypeOfSchedulerConstraint.HOLIDAY);
        assertThat(provider.getTypeOfDay(LocalDate.of(2007, 5, 11))).isEqualTo(TypeOfSchedulerConstraint.WORK);
        assertThat(provider.getTypeOfDay(LocalDate.of(2007, 5, 12))).isEqualTo(TypeOfSchedulerConstraint.PRE_HOLIDAY);
        assertThat(provider.getTypeOfDay(LocalDate.of(2007, 5, 13))).isEqualTo(TypeOfSchedulerConstraint.HOLIDAY);
    }

    @Test
    void lookupNextMonth() throws WorkCalendarProviderException {
        assertThat(provider.getTypeOfDay(LocalDate.of(2007, 6, 1))).isEqualTo(TypeOfSchedulerConstraint.PRE_HOLIDAY);
    }

    private static Holiday holiday(LocalDate date, DayType type) {
        return new Holiday("day-" + date, date, type, false, null);
    }
}
