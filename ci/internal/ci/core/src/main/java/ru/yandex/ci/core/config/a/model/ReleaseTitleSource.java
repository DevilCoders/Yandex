package ru.yandex.ci.core.config.a.model;

import com.fasterxml.jackson.annotation.JsonAlias;
import com.fasterxml.jackson.annotation.JsonProperty;

public enum ReleaseTitleSource {
    @JsonProperty("flow")
    @JsonAlias("FLOW")
    FLOW,
    @JsonProperty("release")
    @JsonAlias("RELEASE")
    RELEASE
}
