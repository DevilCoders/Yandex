package ru.yandex.ci.client.arcanum;

import lombok.Value;

@Value
public class UpdateCheckRequirementRequestDto {

    String system;
    String type;
    Boolean enabled;
    ReasonDto reason;

    @Value
    public static class ReasonDto {
        String text;
    }
}
