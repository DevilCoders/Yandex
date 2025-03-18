package ru.yandex.ci.observer.api.statistics.model;

import javax.annotation.Nullable;

import lombok.Value;

import ru.yandex.ci.observer.core.db.model.check_iterations.CheckIterationEntity;

@Value
public class IterationWithArcanumInfo {
    CheckIterationEntity iteration;
    @Nullable
    String reviewTitle;
    @Nullable
    String reviewDescription;
}
