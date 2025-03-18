package ru.yandex.ci.client.testenv.model;

import com.fasterxml.jackson.annotation.JsonEnumDefaultValue;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Value;

@Value
public class TestenvProject {
    String name;
    String shard;
    String projectType;
    Status status;
    String taskOwner;
    String svnPath;
    String svnServer;

    public enum Status {
        @JsonProperty("working")
        WORKING,
        @JsonProperty("stopped")
        STOPPED,

        @JsonEnumDefaultValue
        UNKNOWN
    }
}
