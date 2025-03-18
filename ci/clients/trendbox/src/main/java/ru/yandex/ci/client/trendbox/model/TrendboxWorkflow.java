package ru.yandex.ci.client.trendbox.model;

import java.time.Instant;

import javax.annotation.Nullable;

import lombok.Value;

@Value
public class TrendboxWorkflow {
    TrendboxScpType scpType;
    String repository;
    String name;
    Instant firstLaunchAt;
    Instant lastLaunchAt;
    @Nullable
    Instant lastSuccessAt;
}

