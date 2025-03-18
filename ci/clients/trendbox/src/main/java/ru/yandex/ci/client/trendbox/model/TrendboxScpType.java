package ru.yandex.ci.client.trendbox.model;

import com.fasterxml.jackson.annotation.JsonProperty;

public enum TrendboxScpType {
    @JsonProperty("github")
    GITHUB,
    @JsonProperty("bitbucket_server")
    BITBUCKET,
    @JsonProperty("arcanum")
    ARCADIA
}
