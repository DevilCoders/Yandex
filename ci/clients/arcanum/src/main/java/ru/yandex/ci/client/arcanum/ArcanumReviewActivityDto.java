package ru.yandex.ci.client.arcanum;

import java.time.LocalDateTime;
import java.util.List;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Value;

@Value
public class ArcanumReviewActivityDto {

    List<TeRun> activities;

    @Value
    public static class TeRun {
        @Nullable
        String action;

        @Nullable
        LocalDateTime actionTime;

        @Nullable
        @JsonProperty("testEnvCardActivityApiModel")
        TestEnvCardActivityApiModel testEnvCardActivityApiModel;
    }

    @Value
    public static class TestEnvCardActivityApiModel {
        @JsonProperty("checkId")
        String checkId;
    }
}
