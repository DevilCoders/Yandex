package ru.yandex.ci.client.tsum;

import com.fasterxml.jackson.annotation.JsonEnumDefaultValue;
import lombok.Value;

@Value
public class TsumDeliveryMachines {
    String stageGroupId;
    String title;
    VcsSettings vcsSettings;

    @Value
    public static class VcsSettings {
        VcsType type;
    }

    public enum VcsType {
        ARCADIA,
        GITHUB,
        BITBUCKET,

        @JsonEnumDefaultValue
        UNKNOWN
    }
}
