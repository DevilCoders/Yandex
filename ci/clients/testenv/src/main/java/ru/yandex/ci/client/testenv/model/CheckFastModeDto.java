package ru.yandex.ci.client.testenv.model;

import com.fasterxml.jackson.annotation.JsonProperty;

public enum CheckFastModeDto {
    @JsonProperty("auto")
    AUTO,
    @JsonProperty("parallel")
    PARALLEL,
    @JsonProperty("parallel-no-fail-fast")
    PARALLEL_NO_FAIL_FAST,
    @JsonProperty("disabled")
    DISABLED,
    @JsonProperty("fast-only")
    FAST_ONLY,
    @JsonProperty("sequential")
    SEQUENTIAL
}
