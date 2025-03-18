package ru.yandex.ci.flow.engine.runtime.calendar;

import java.time.DayOfWeek;
import java.time.Instant;
import java.time.LocalDate;
import java.time.LocalDateTime;
import java.time.ZoneId;
import java.time.ZoneOffset;
import java.time.temporal.ChronoUnit;
import java.util.concurrent.TimeUnit;

import javax.annotation.Nullable;

import com.google.common.annotations.VisibleForTesting;

import ru.yandex.ci.flow.engine.definition.common.JobSchedulerConstraintEntity;
import ru.yandex.ci.flow.engine.definition.common.SchedulerIntervalEntity;
import ru.yandex.ci.flow.engine.definition.common.TypeOfSchedulerConstraint;
import ru.yandex.ci.flow.engine.definition.common.WeekSchedulerConstraintEntity;

/**
 * Сервис работает для часового пояса Europe/Moscow. От этого зависят проверки на время запуска.
 * Пример: для даты 27.03.2019 23:00 UTS четверг рабочий день
 * в часовом поясе МСК будет 28.03.2019 02:00 пятница предвыходный день
 */
public class WorkCalendarService {
    private static final int MAX_DAY_WAITING_COUNT = 60;
    private static final ZoneId MOSCOW_ZONE = ZoneId.of("Europe/Moscow");

    private final WorkCalendarProvider workCalendarProvider;

    public WorkCalendarService(WorkCalendarProvider workCalendarProvider) {
        this.workCalendarProvider = workCalendarProvider;
    }

    public AllowedDate getNextAllowedDate(
            @Nullable JobSchedulerConstraintEntity schedulerConstraint
    ) throws WorkCalendarProviderException {
        if (schedulerConstraint == null || schedulerConstraint.isEmptyWeekConstraints()) {
            return AllowedDate.anyDateAllowed();
        }

        Instant nowInstant = Instant.now();
        LocalDate currentDay = nowInstant.atZone(MOSCOW_ZONE).toLocalDate();
        TypeOfSchedulerConstraint typeOfSchedulerConstraint = workCalendarProvider.getTypeOfDay(currentDay);

        Instant result = tryFindInstantForCurrentDay(schedulerConstraint,
                typeOfSchedulerConstraint,
                nowInstant);

        if (result != null) {
            return AllowedDate.of(result);
        }
        int numberOfIteration = 0;
        while (result == null && numberOfIteration <= MAX_DAY_WAITING_COUNT) {
            currentDay = currentDay.plusDays(1);
            ++numberOfIteration;

            typeOfSchedulerConstraint = workCalendarProvider.getTypeOfDay(currentDay);

            result = tryFindStartInstantForDay(schedulerConstraint,
                    typeOfSchedulerConstraint,
                    currentDay);
        }

        return result != null ? AllowedDate.of(result) : AllowedDate.dateNotFound();
    }

    /**
     * Функция поиска времени запуска в течении дня не ранее указанного времени.
     * Существуют следующие ситуации:
     * - запуск невозможен по типу дня
     * - запуск невозможен из-за выхода за верхнюю границу допустимого интервала
     * - запуск сейчас невозможен, но доступен позже в этот день, на нижней границе допустимого интервала
     * - запуск возможен сейчас
     *
     * @param constraints расписание запуска
     * @param typeOfDay   тип ограничения для расписания запуксков
     * @param nowInstant  время, относительно которого производится проверка
     * @return время запуска, null если запуск в этот день невозможен
     */
    @Nullable
    @VisibleForTesting
    public Instant tryFindInstantForCurrentDay(
            JobSchedulerConstraintEntity constraints,
            TypeOfSchedulerConstraint typeOfDay,
            Instant nowInstant
    ) {
        LocalDate currentDay = nowInstant.atZone(MOSCOW_ZONE).toLocalDate();

        SchedulerIntervalEntity interval = getSchedulerIntervalForDay(
                constraints, typeOfDay, currentDay.getDayOfWeek());

        if (interval != null) {
            LocalDateTime nowTime = LocalDateTime.ofInstant(nowInstant, MOSCOW_ZONE);

            long currentDayMinutes = TimeUnit.HOURS.toMinutes(nowTime.getHour()) + nowTime.getMinute();
            if (currentDayMinutes <= interval.getMinutesTo()) {
                if (interval.between(currentDayMinutes)) {
                    return nowInstant.truncatedTo(ChronoUnit.MINUTES);
                } else {
                    LocalDateTime scheduleTime = currentDay.atStartOfDay().plusMinutes(interval.getMinutesFrom());
                    ZoneOffset moscowOffset = MOSCOW_ZONE.getRules().getOffset(scheduleTime);
                    return scheduleTime.toInstant(moscowOffset);
                }
            }
        }
        return null;
    }

    /**
     * Функция поиска времени запуска в указанный день.
     * Существуют следующие ситуации:
     * - запуск невозможен по типу дня
     * - запуск в этот день возможен
     *
     * @param constraints расписание запуска
     * @param typeOfDay   тип ограничения для расписания запуксков
     * @param currentDay  проверяемый день
     * @return время запуска, null если запуск в этот день невозможен
     */
    @Nullable
    @VisibleForTesting
    public Instant tryFindStartInstantForDay(
            JobSchedulerConstraintEntity constraints,
            TypeOfSchedulerConstraint typeOfDay,
            LocalDate currentDay
    ) {
        SchedulerIntervalEntity interval = getSchedulerIntervalForDay(
                constraints,
                typeOfDay,
                currentDay.getDayOfWeek());
        if (interval != null) {
            LocalDateTime scheduleTime = currentDay.atStartOfDay().plusMinutes(interval.getMinutesFrom());
            ZoneOffset moscowOffset = MOSCOW_ZONE.getRules().getOffset(scheduleTime);
            return scheduleTime.toInstant(moscowOffset);
        }
        return null;
    }

    @Nullable
    private static SchedulerIntervalEntity getSchedulerIntervalForDay(
            JobSchedulerConstraintEntity constraints,
            TypeOfSchedulerConstraint typeOfDay,
            DayOfWeek dayOfWeek
    ) {
        WeekSchedulerConstraintEntity weekConstraint = constraints.getWeekConstraintsForType(typeOfDay);

        if (weekConstraint == null && TypeOfSchedulerConstraint.PRE_HOLIDAY.equals(typeOfDay)) {
            weekConstraint = constraints.getWeekConstraintsForType(TypeOfSchedulerConstraint.WORK);
        }

        if (weekConstraint != null) {
            return weekConstraint.getAllowedDayOfWeekIntervals()
                    .get(dayOfWeek);
        }
        return null;
    }
}
