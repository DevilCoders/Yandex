package ru.yandex.ci.flow.engine.definition.common;

import java.util.concurrent.TimeUnit;

import ru.yandex.ci.ydb.Persisted;

/**
 * Разрешённый интервал запуска задачи.
 * minutesFrom - количество минут с начала дня до которого запуск задачи запрещён
 * minutesTo - количество минут с начала дня после которого запуск задачи запрещён
 */
@Persisted
public class SchedulerIntervalEntity {
    private static final int MAX_MINUTES_IN_DAY = (int) TimeUnit.DAYS.toMinutes(1) - 1;
    private final int minutesFrom;
    private final int minutesTo;

    public SchedulerIntervalEntity() {
        this.minutesFrom = 0;
        this.minutesTo = MAX_MINUTES_IN_DAY;
    }

    public SchedulerIntervalEntity(int minutesFrom, int minutesTo) {
        this.minutesFrom = truncateMinutes(minutesFrom);
        this.minutesTo = truncateMinutes(minutesTo);
    }

    public int getMinutesFrom() {
        return minutesFrom;
    }

    public int getMinutesTo() {
        return minutesTo;
    }

    @Override
    public String toString() {
        return "(" + minutesFrom + ", " + minutesTo + ")";
    }

    public boolean between(long time) {
        return minutesFrom <= time && time <= minutesTo;
    }

    private int truncateMinutes(int minutes) {
        if (minutes < 0) {
            return 0;
        }
        return Math.min(minutes, MAX_MINUTES_IN_DAY);
    }
}
