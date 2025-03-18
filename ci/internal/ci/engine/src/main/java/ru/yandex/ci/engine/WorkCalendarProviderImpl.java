package ru.yandex.ci.engine;

import java.time.LocalDate;
import java.util.List;
import java.util.concurrent.ExecutionException;

import com.google.common.base.Preconditions;
import com.google.common.cache.Cache;
import com.google.common.cache.CacheBuilder;
import com.google.common.cache.CacheLoader;
import com.google.common.cache.LoadingCache;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.client.calendar.CalendarClient;
import ru.yandex.ci.client.calendar.api.Country;
import ru.yandex.ci.client.calendar.api.DayType;
import ru.yandex.ci.client.calendar.api.Holiday;
import ru.yandex.ci.flow.engine.definition.common.TypeOfSchedulerConstraint;
import ru.yandex.ci.flow.engine.runtime.calendar.WorkCalendarProvider;
import ru.yandex.ci.flow.engine.runtime.calendar.WorkCalendarProviderException;
import ru.yandex.ci.util.ExceptionUtils;

@Slf4j
@RequiredArgsConstructor
public class WorkCalendarProviderImpl implements WorkCalendarProvider {
    private final CalendarClient calendarClient;

    private final LoadingCache<LocalDate, List<Holiday>> holidays = CacheBuilder.newBuilder()
            .maximumSize(2)
            .build(CacheLoader.from(this::loadMonthHolidays));

    private final Cache<LocalDate, TypeOfSchedulerConstraint> cache = CacheBuilder.newBuilder()
            .maximumSize(7)
            .build();

    @Override
    public TypeOfSchedulerConstraint getTypeOfDay(LocalDate day) throws WorkCalendarProviderException {
        try {
            return cache.get(day, () -> {
                var calendarDay = getCalendarDay(day);
                return switch (calendarDay.getType()) {
                    case WEEKDAY -> {
                        var nextDay = getCalendarDay(day.plusDays(1));
                        if (nextDay.getType() == DayType.WEEKEND || nextDay.getType() == DayType.HOLIDAY) {
                            yield TypeOfSchedulerConstraint.PRE_HOLIDAY;
                        }
                        yield TypeOfSchedulerConstraint.WORK;
                    }
                    case HOLIDAY, WEEKEND -> TypeOfSchedulerConstraint.HOLIDAY;
                };
            });
        } catch (ExecutionException e) {
            if (e.getCause() instanceof WorkCalendarProviderException) {
                throw ((WorkCalendarProviderException) e.getCause());
            }
            throw new WorkCalendarProviderException(ExceptionUtils.unwrap(e));
        }
    }

    private Holiday getCalendarDay(LocalDate day) throws ExecutionException {
        List<Holiday> dayList = this.holidays.get(day.withDayOfMonth(1));
        return dayList.stream()
                .filter(h -> h.getDate().equals(day))
                .findFirst()
                .orElseThrow();
    }

    private List<Holiday> loadMonthHolidays(LocalDate startOfMonth) {
        Preconditions.checkArgument(startOfMonth.getDayOfMonth() == 1);
        log.info("Loading holidays for {}", startOfMonth.getMonth());
        return calendarClient.getHolidays(startOfMonth, startOfMonth.plusMonths(1), Country.RUSSIA)
                .getHolidays();
    }
}
