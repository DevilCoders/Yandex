package ru.yandex.ci.core.config.a.model.auto;


import java.time.DayOfWeek;
import java.time.LocalTime;
import java.time.ZoneId;
import java.util.Set;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import com.google.common.base.Preconditions;
import lombok.Value;

@Value
public class Schedule {
    private static final ZoneId DEFAULT_TIMEZONE = ZoneId.of("Europe/Moscow");

    @Nonnull
    @JsonDeserialize(converter = LocalTimeRangeDeserializer.class)
    @JsonSerialize(converter = LocalTimeRangeSerializer.class)
    TimeRange time;

    @Nullable
    @JsonDeserialize(converter = DaysOfWeekRangeDeserializer.class)
    @JsonSerialize(converter = DaysOfWeekRangeSerializer.class)
    Set<DayOfWeek> days;

    @Nullable
    @JsonProperty("day-type")
    DayType dayType;

    public Schedule(@Nonnull TimeRange time, @Nullable Set<DayOfWeek> days, @Nullable DayType dayType) {
        this.time = time;
        this.days = days;
        this.dayType = dayType;
    }

    public Schedule(@JsonProperty("start")
                    @JsonDeserialize(converter = LocalTimeSimpleDeserializer.class)
                    @Nullable LocalTime start,

                    @JsonProperty("end")
                    @JsonDeserialize(converter = LocalTimeSimpleDeserializer.class)
                    @Nullable LocalTime end,

                    @JsonProperty("time") @Nullable TimeRange time,
                    @JsonProperty("day-type") @Nullable DayType dayType,
                    @JsonProperty("days") @Nullable Set<DayOfWeek> days
    ) {
        Preconditions.checkArgument((start != null && end != null) || time != null, "time is mandatory");
        this.days = days;

        this.time = time != null
                ? time
                : TimeRange.of(start, end, DEFAULT_TIMEZONE);
        this.dayType = dayType;
    }

    @JsonIgnore
    public ZoneId getTimezone() {
        return time.getZoneId();
    }

    @JsonIgnore
    @Nonnull
    public Set<DayOfWeek> getDaysOrAll() {
        if (days == null) {
            return Set.of(DayOfWeek.values());
        }
        return days;
    }
}
