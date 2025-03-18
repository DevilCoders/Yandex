package ru.yandex.ci.client.testenv.model;

import java.util.List;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonEnumDefaultValue;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Value;

@Value
public class TestenvJob {
    String name;
    String testModulePath;
    String jobType;
    Status status;
    Frequency frequency;
    @Nullable
    String jobStoppedReason;
    List<String> tags;
    List<DependencyJob> childJobs;
    List<DependencyJob> parentJobs;
    String testModuleName;

    @Value
    public static class Frequency {
        Schedule schedule;
    }

    public enum Schedule {
        @JsonProperty("run_if_delay_n_minutes")
        RUN_IF_DELAY_N_MINUTES,
        @JsonProperty("run_n_minutes_after_last_run")
        RUN_N_MINUTES_AFTER_LAST_RUN,
        @JsonProperty("check_each_commit")
        CHECK_EACH_COMMIT,
        @JsonProperty("every_n_commit")
        EVERY_N_COMMIT,
        @JsonProperty("lazy")
        LAZY,
        @JsonProperty("defined_by_code")
        DEFINED_BY_CODE,

        @JsonEnumDefaultValue
        UNKNOWN
    }

    public enum Status {
        @JsonProperty("working")
        WORKING,
        @JsonProperty("stopped")
        STOPPED,

        @JsonEnumDefaultValue
        UNKNOWN
    }

    @Value
    public static class DependencyJob {
        String name;
        List<DependencyJob> items;
    }

}
