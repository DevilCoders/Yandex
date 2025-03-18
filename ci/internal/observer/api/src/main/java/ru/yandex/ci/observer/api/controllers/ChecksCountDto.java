package ru.yandex.ci.observer.api.controllers;

import java.time.Instant;
import java.util.Map;

import lombok.Value;

import ru.yandex.ci.storage.core.CheckOuterClass;

@Value
public class ChecksCountDto {

    Instant timestamp;
    Map<CheckOuterClass.CheckType, Long> checksCount;

}
