package ru.yandex.ci.client.sandbox.api;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.google.gson.annotations.SerializedName;

public enum TaskRequirementsDns {
    @JsonProperty("default")
    @SerializedName("default")
    DEFAULT,
    @JsonProperty("local")
    @SerializedName("local")
    LOCAL,
    @JsonProperty("dns64")
    @SerializedName("dns64")
    DNS64,
}
