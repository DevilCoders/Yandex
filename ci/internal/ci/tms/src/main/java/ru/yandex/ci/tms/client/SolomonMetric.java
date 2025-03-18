package ru.yandex.ci.tms.client;

import java.util.Map;

import lombok.Builder;
import lombok.Singular;
import lombok.Value;

@Value
@Builder
public class SolomonMetric {
    @Singular
    Map<String, String> labels;

    double value;
}
