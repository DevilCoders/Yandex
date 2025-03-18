package ru.yandex.ci.client.calendar.api;

import java.time.LocalDate;

import javax.annotation.Nullable;

import lombok.Value;

@Value
public class Holiday {
    String name;
    LocalDate date;
    DayType type;

    boolean isTransfer;
    @Nullable
    LocalDate transferDate;

}
